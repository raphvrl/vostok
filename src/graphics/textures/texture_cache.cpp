#include "graphics/textures/texture_cache.hpp"

#include "core/logger/logger.hpp"
#include "graphics/gpu.hpp"
#include "graphics/textures/texture.hpp"

#include <algorithm>
#include <set>

namespace vostok::graphics
{

auto TextureCache::create(GPUHandle *gpu, const CacheConfig &config)
    -> std::expected<std::unique_ptr<TextureCache>, std::string>
{
    try {
        if (config.maxMemoryBytes == 0) {
            return std::unexpected{ "Max memory bytes cannot be 0" };
        }

        if (config.maxTextures == 0) {
            return std::unexpected{ "Max textures cannot be 0" };
        }

        if (config.evictionThreshold <= 0.0F ||
            config.evictionThreshold >= 1.0F) {
            return std::unexpected{
                "Eviction threshold must be between 0.0 and 1.0"
            };
        }

        auto cache =
            std::unique_ptr<TextureCache>(new TextureCache(gpu, config));

        Logger::info(
            "TextureCache created: {}MB max memory, {} max textures",
            config.maxMemoryBytes / static_cast<size_t>(1024 * 1024),
            config.maxTextures
        );

        return cache;
    } catch (const std::exception &e) {
        return std::unexpected{
            std::format("Failed to create TextureCache: {}", e.what())
        };
    }
}

TextureCache::TextureCache(GPUHandle *gpu, const CacheConfig &config)
    : m_gpu(gpu),
      m_config(config),
      m_lastStatsUpdate(std::chrono::steady_clock::now())
{
    m_entries.reserve(config.maxTextures);
    m_lruMap.reserve(config.maxTextures);

    Logger::info(
        "TextureCache initialized with {}MB max memory",
        config.maxMemoryBytes / static_cast<size_t>(1024 * 1024)
    );
}

auto TextureCache::get(const fs::path &path) -> std::optional<CacheEntry *>
{
    if (path.empty()) {
        Logger::warning("Attempted to get texture with empty path");
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entries.find(path);
    if (it == m_entries.end()) {
        m_stats.cacheMisses++;
        return std::nullopt;
    }

    auto &entry = it->second;
    entry.lastAccess = std::chrono::steady_clock::now();
    entry.accessCount++;
    m_stats.cacheHits++;

    if (m_config.enableLRU) {
        updateLRU(path);
    }

    return &entry;
}

auto TextureCache::put(
    const fs::path &path,
    Texture &&texture,
    const std::string &debugName
) -> bool
{
    if (path.empty()) {
        Logger::error("Cannot put texture with empty path");
        return false;
    }

    if (!texture) {
        Logger::error("Cannot put null texture for path: {}", path.string());
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    size_t textureMemory = calculateMemoryUsage(texture);
    if (textureMemory == 0) {
        Logger::error(
            "Failed to calculate memory usage for texture: {}",
            path.string()
        );
        return false;
    }

    if (textureMemory > m_config.maxMemoryBytes) {
        Logger::warning(
            "Texture {} too large ({}MB) for cache (max: {}MB)",
            path.string(),
            textureMemory / static_cast<size_t>(1024 * 1024),
            m_config.maxMemoryBytes / static_cast<size_t>(1024 * 1024)
        );
        return false;
    }

    if (m_entries.size() >= m_config.maxTextures ||
        (m_stats.totalMemoryUsage + textureMemory) > m_config.maxMemoryBytes) {
        evictLRU(textureMemory);
    }

    CacheEntry entry;
    entry.texture = std::move(texture);
    entry.filePath = path;
    entry.memoryUsage = textureMemory;
    entry.lastAccess = std::chrono::steady_clock::now();
    entry.creationTime = std::chrono::steady_clock::now();
    entry.debugName = debugName.empty() ? path.filename().string() : debugName;

    if (entry.texture) {
        entry.width = entry.texture->getWidth();
        entry.height = entry.texture->getHeight();
        entry.format = entry.texture->getFormat();

        if (entry.width == 0 || entry.height == 0) {
            Logger::warning(
                "Texture {} has invalid dimensions: {}x{}",
                path.string(),
                entry.width,
                entry.height
            );
        }
    }

    try {
        m_entries[path] = std::move(entry);
        m_stats.totalMemoryUsage += textureMemory;
        m_stats.totalTextures++;

        if (m_config.enableLRU) {
            updateLRU(path);
        }

        logCacheOperation("PUT", path, textureMemory);
        return true;

    } catch (const std::exception &e) {
        Logger::error(
            "Failed to insert texture {} into cache: {}",
            path.string(),
            e.what()
        );
        return false;
    }
}

void TextureCache::remove(const fs::path &path)
{
    if (path.empty()) {
        Logger::warning("Attempted to remove texture with empty path");
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entries.find(path);
    if (it == m_entries.end()) {
        Logger::trace(
            "Attempted to remove non-existent texture: {}",
            path.string()
        );
        return;
    }

    evictEntry(path);
    Logger::trace("Removed texture from cache: {}", path.string());
}

auto TextureCache::evictLRU(size_t requiredMemory) -> size_t
{
    if (m_lruList.empty()) {
        Logger::trace("LRU list is empty, nothing to evict");
        return 0;
    }

    size_t freedMemory = 0;
    auto it = m_lruList.begin();

    while (it != m_lruList.end() && freedMemory < requiredMemory) {
        const auto &path = *it;
        auto entryIt = m_entries.find(path);

        if (entryIt != m_entries.end() && !entryIt->second.isPinned) {
            size_t entryMemory = entryIt->second.memoryUsage;
            evictEntry(path);
            freedMemory += entryMemory;

            Logger::trace(
                "Evicted texture {} ({}MB) from cache",
                path.string(),
                entryMemory / static_cast<size_t>(1024 * 1024)
            );
        }

        ++it;
    }

    if (freedMemory > 0) {
        m_stats.evictions++;
        Logger::debug(
            "LRU eviction freed {}MB",
            freedMemory / static_cast<size_t>(1024 * 1024)
        );
    }

    return freedMemory;
}

auto TextureCache::evictOldest(size_t requiredMemory) -> size_t
{
    if (m_entries.empty()) {
        return 0;
    }

    size_t freedMemory = 0;

    std::set<std::pair<std::chrono::steady_clock::time_point, fs::path>>
        ageSorted;

    for (const auto &[path, entry] : m_entries) {
        if (!entry.isPinned) {
            ageSorted.emplace(entry.creationTime, path);
        }
    }

    for (const auto &[creationTime, path] : ageSorted) {
        if (freedMemory >= requiredMemory) {
            break;
        }

        auto it = m_entries.find(path);
        if (it != m_entries.end()) {
            size_t entryMemory = it->second.memoryUsage;
            evictEntry(path);
            freedMemory += entryMemory;
        }
    }

    return freedMemory;
}

auto TextureCache::evictByMemoryUsage(size_t targetMemory) -> size_t
{
    if (m_stats.totalMemoryUsage <= targetMemory) {
        Logger::trace(
            "Current memory usage ({}MB) already below target ({}MB)",
            m_stats.totalMemoryUsage / static_cast<size_t>(1024 * 1024),
            targetMemory / static_cast<size_t>(1024 * 1024)
        );
        return 0;
    }

    size_t memoryToFree = m_stats.totalMemoryUsage - targetMemory;
    size_t freedMemory = 0;

    if (m_config.enableLRU && !m_lruList.empty()) {
        freedMemory = evictLRU(memoryToFree);
    }

    if (freedMemory < memoryToFree) {
        size_t remainingToFree = memoryToFree - freedMemory;
        freedMemory += evictOldest(remainingToFree);
    }

    if (freedMemory > 0) {
        Logger::debug(
            "Memory-based eviction freed {}MB, target: {}MB, current: {}MB",
            freedMemory / static_cast<size_t>(1024 * 1024),
            targetMemory / static_cast<size_t>(1024 * 1024),
            m_stats.totalMemoryUsage / static_cast<size_t>(1024 * 1024)
        );
    }

    return freedMemory;
}

void TextureCache::pinTexture(const fs::path &path)
{
    if (path.empty()) {
        Logger::warning("Attempted to pin texture with empty path");
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entries.find(path);
    if (it == m_entries.end()) {
        Logger::warning(
            "Attempted to pin non-existent texture: {}",
            path.string()
        );
        return;
    }

    it->second.isPinned = true;
    Logger::trace("Pinned texture: {}", path.string());
}

void TextureCache::unpinTexture(const fs::path &path)
{
    if (path.empty()) {
        Logger::warning("Attempted to unpin texture with empty path");
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entries.find(path);
    if (it == m_entries.end()) {
        Logger::warning(
            "Attempted to unpin non-existent texture: {}",
            path.string()
        );
        return;
    }

    it->second.isPinned = false;
    Logger::trace("Unpinned texture: {}", path.string());
}

auto TextureCache::incrementReference(const fs::path &path) -> size_t
{
    if (path.empty()) {
        Logger::warning("Attempted to increment reference with empty path");
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entries.find(path);
    if (it == m_entries.end()) {
        Logger::warning(
            "Attempted to increment reference for non-existent texture: {}",
            path.string()
        );
        return 0;
    }

    it->second.referenceCount++;

    Logger::trace(
        "Incremented reference for texture {}: {} references",
        path.string(),
        it->second.referenceCount
    );

    return it->second.referenceCount;
}

auto TextureCache::decrementReference(const fs::path &path) -> size_t
{
    if (path.empty()) {
        Logger::warning("Attempted to decrement reference with empty path");
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entries.find(path);
    if (it == m_entries.end()) {
        Logger::warning(
            "Attempted to decrement reference for non-existent texture: {}",
            path.string()
        );
        return 0;
    }

    if (it->second.referenceCount > 0) {
        it->second.referenceCount--;
    }

    Logger::trace(
        "Decremented reference for texture {}: {} references remaining",
        path.string(),
        it->second.referenceCount
    );

    if (it->second.referenceCount == 0 && !it->second.isPinned) {
        Logger::trace(
            "Texture {} has no more references, removing from cache",
            path.string()
        );
        evictEntry(path);
    }

    return it->second.referenceCount;
}

auto TextureCache::getMemoryUsage() const -> size_t
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats.totalMemoryUsage;
}

auto TextureCache::getTextureCount() const -> size_t
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats.totalTextures;
}

auto TextureCache::getStats() const -> const CacheStats &
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_stats;
}

auto TextureCache::getStatsWithHitRate() const -> CacheStats
{
    std::lock_guard<std::mutex> lock(m_mutex);

    CacheStats stats = m_stats;

    size_t totalAccesses = stats.cacheHits + stats.cacheMisses;
    if (totalAccesses > 0) {
        stats.hitRate =
            static_cast<f32>(stats.cacheHits) / static_cast<f32>(totalAccesses);
    }

    return stats;
}

void TextureCache::update()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (static_cast<f32>(m_stats.totalMemoryUsage) >
        static_cast<f32>(m_config.maxMemoryBytes) *
            m_config.evictionThreshold) {
        auto targetMemory = static_cast<size_t>(
            static_cast<f32>(m_config.maxMemoryBytes) * 0.7F
        );
        evictByMemoryUsage(targetMemory);

        Logger::debug(
            "Auto-eviction triggered: target {}MB, current {}MB",
            targetMemory / static_cast<size_t>(1024 * 1024),
            m_stats.totalMemoryUsage / static_cast<size_t>(1024 * 1024)
        );
    }

    auto now = std::chrono::steady_clock::now();
    if (now - m_lastStatsUpdate > std::chrono::seconds(5)) {
        updateStats();
        m_lastStatsUpdate = now;
    }
}

void TextureCache::cleanup()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    size_t initialCount = m_stats.totalTextures;
    size_t initialMemory = m_stats.totalMemoryUsage;

    m_entries.clear();
    m_lruList.clear();
    m_lruMap.clear();

    m_stats.totalTextures = 0;
    m_stats.totalMemoryUsage = 0;

    Logger::debug(
        "Cache cleanup completed: removed {} textures, freed {}MB",
        initialCount,
        initialMemory / static_cast<size_t>(1024 * 1024)
    );
}

void TextureCache::updateLRU(const fs::path &path)
{
    auto it = m_lruMap.find(path);
    if (it != m_lruMap.end()) {
        m_lruList.erase(it->second);
    }

    m_lruList.push_front(path);
    m_lruMap[path] = m_lruList.begin();
}

void TextureCache::evictEntry(const fs::path &path)
{
    auto it = m_entries.find(path);
    if (it == m_entries.end()) {
        return;
    }

    auto &entry = it->second;

    if ((m_gpu != nullptr) && entry.texture) {
        Logger::trace(
            "Unregistering texture {} from bindless system (index: {})",
            path.string(),
            entry.texture->getBindlessIndex()
        );

        auto result = m_gpu->destroyTexture(entry.texture);
        if (!result) {
            Logger::warning(
                "Failed to unregister texture {} from bindless: {}",
                path.string(),
                result.error()
            );
        } else {
            Logger::trace(
                "Successfully unregistered texture {} from bindless system",
                path.string()
            );
        }
    }

    m_stats.totalMemoryUsage -= it->second.memoryUsage;
    m_stats.totalTextures--;

    auto lruIt = m_lruMap.find(path);
    if (lruIt != m_lruMap.end()) {
        m_lruList.erase(lruIt->second);
        m_lruMap.erase(lruIt);
    }

    m_entries.erase(it);
}

auto TextureCache::calculateMemoryUsage(const Texture &texture) -> size_t
{
    if (!texture) {
        return 0;
    }

    u32 width = texture->getWidth();
    u32 height = texture->getHeight();

    if (width == 0 || height == 0) {
        Logger::warning("Invalid texture dimensions: {}x{}", width, height);
        return 0;
    }

    size_t pixelCount = static_cast<size_t>(width) * height;
    size_t bytesPerPixel = 0;

    switch (texture->getFormat()) {
        case ImageFormat::R8_UNORM:
            bytesPerPixel = 1;
            break;
        case ImageFormat::R8G8B8_UNORM:
            bytesPerPixel = 3;
            break;
        case ImageFormat::R8G8B8A8_UNORM:
        case ImageFormat::R8G8B8A8_SRGB:
            bytesPerPixel = 4;
            break;
        case ImageFormat::R32G32B32A32_SFLOAT:
            bytesPerPixel = 16;
            break;
        default:
            Logger::warning("Unknown texture format, using 4 bytes per pixel");
            bytesPerPixel = 4;
            break;
    }

    size_t mipmapMultiplier = 1;
    u32 mipLevels = texture->getMipLevels();
    if (mipLevels > 1) {
        mipmapMultiplier = std::min(4U, mipLevels);
    }

    size_t totalMemory = pixelCount * bytesPerPixel * mipmapMultiplier;

    if (totalMemory == 0) {
        Logger::error(
            "Calculated memory usage is 0 for texture {}x{}",
            width,
            height
        );
        return 0;
    }

    return totalMemory;
}

void TextureCache::logCacheOperation(
    const std::string &operation,
    const fs::path &path,
    size_t memoryUsage
)
{
    Logger::trace(
        "Cache {}: {} ({}MB) - Total: {}MB, {} textures",
        operation,
        path.string(),
        memoryUsage / static_cast<size_t>(1024 * 1024),
        m_stats.totalMemoryUsage / static_cast<size_t>(1024 * 1024),
        m_stats.totalTextures
    );
}

void TextureCache::updateStats()
{
    if (m_stats.totalTextures > 0) {
        auto memoryPerTexture = static_cast<f32>(m_stats.totalMemoryUsage) /
                                static_cast<f32>(m_stats.totalTextures);
        Logger::trace(
            "Cache stats: {} textures, {:.2f}MB avg per texture, {:.1f}% hit "
            "rate",
            m_stats.totalTextures,
            memoryPerTexture / (1024 * 1024),
            m_stats.hitRate * 100.0F
        );
    }
}

void TextureCache::resetStats()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_stats = CacheStats{};
    m_lastStatsUpdate = std::chrono::steady_clock::now();

    Logger::debug("Cache statistics reset");
}

void TextureCache::preloadTextures(const std::vector<fs::path> &paths)
{
    if (paths.empty()) {
        Logger::trace("No textures to preload");
        return;
    }

    Logger::debug("Preloading {} textures", paths.size());

    for (const auto &path : paths) {
        if (path.empty()) {
            Logger::warning("Skipping empty path in preload");
            continue;
        }

        auto existing = get(path);
        if (existing) {
            Logger::trace("Texture already in cache: {}", path.string());
            continue;
        }

        Logger::trace("Texture queued for preload: {}", path.string());
    }
}

} // namespace vostok::graphics