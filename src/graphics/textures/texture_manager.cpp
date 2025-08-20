#include "vostok/graphics/textures/texture_manager.hpp"

#include "core/logger/logger.hpp"
#include "graphics/textures/texture_loader.hpp"

#include <format>

namespace fs = std::filesystem;

namespace vostok::graphics
{

auto TextureManagerHandle::create(GPUHandle *gpu, const Config &config)
    -> std::expected<std::unique_ptr<TextureManagerHandle>, std::string>
{
    try {
        if (gpu == nullptr) {
            return std::unexpected{ "GPU handle cannot be null" };
        }

        TextureCache::CacheConfig cacheConfig;
        cacheConfig.maxMemoryBytes = config.maxMemoryBytes;
        cacheConfig.maxTextures = config.maxTextures;
        cacheConfig.enableLRU = config.enableLRU;
        cacheConfig.enableAsyncEviction = config.enableAsyncEviction;
        cacheConfig.evictionThreshold = config.evictionThreshold;

        auto cacheResult = TextureCache::create(gpu, cacheConfig);
        if (!cacheResult) {
            return std::unexpected{ std::format(
                "Failed to create texture cache: {}",
                cacheResult.error()
            ) };
        }

        auto manager = std::unique_ptr<TextureManagerHandle>(
            new TextureManagerHandle(gpu, config)
        );
        manager->m_cache = std::move(cacheResult.value());

        Logger::info(
            "TextureManager created successfully with {}MB max memory, {} max "
            "textures",
            config.maxMemoryBytes / static_cast<size_t>(1024 * 1024),
            config.maxTextures
        );

        return manager;
    } catch (const std::exception &e) {
        return std::unexpected{
            std::format("Failed to create TextureManager: {}", e.what())
        };
    }
}

TextureManagerHandle::TextureManagerHandle(GPUHandle *gpu, const Config &config)
    : m_gpu(gpu),
      m_config(config)
{
    Logger::debug(
        "TextureManager initialized with debug prefix: {}",
        config.debugPrefix
    );
}

auto TextureManagerHandle::getTexture(const fs::path &path)
    -> std::expected<TextureCache::CacheEntry *, std::string>
{
    if (path.empty()) {
        return std::unexpected{ "Texture path cannot be empty" };
    }

    auto entry = m_cache->get(path);
    if (entry) {
        m_cache->incrementReference(path);
        return entry.value();
    }

    return std::unexpected{ "Texture not found in cache" };
}

auto TextureManagerHandle::loadTexture(
    const fs::path &path,
    const std::string &debugName
) -> std::expected<TextureCache::CacheEntry *, std::string>
{
    if (path.empty()) {
        return std::unexpected{ "Texture path cannot be empty" };
    }

    auto textureResult = TextureLoader::loadFromFile(m_gpu, path);
    if (!textureResult) {
        auto error = std::format(
            "Failed to load texture {}: {}",
            path.string(),
            textureResult.error()
        );
        m_lastError = error;

        Logger::error("Texture loading failed: {}", error);

        if (m_config.createPlaceholderOnError) {
            Logger::warning(
                "Creating placeholder texture for: {}",
                path.string()
            );
            auto placeholder = createPlaceholderTexture();
            if (placeholder) {
                auto debugNameFinal =
                    debugName.empty() ? path.filename().string() : debugName;
                if (m_cache
                        ->put(path, std::move(placeholder), debugNameFinal)) {
                    auto cacheEntry = m_cache->get(path);
                    if (cacheEntry) {
                        return cacheEntry.value();
                    }
                }
            }
        }

        return std::unexpected{ error };
    }

    auto texture = std::move(textureResult.value());
    auto debugNameFinal =
        debugName.empty() ? path.filename().string() : debugName;

    if (!m_cache->put(path, std::move(texture), debugNameFinal)) {
        return std::unexpected{
            std::format("Failed to cache texture: {}", path.string())
        };
    }

    auto cacheEntry = m_cache->get(path);
    if (cacheEntry) {
        return cacheEntry.value();
    }

    return std::unexpected{ "Failed to retrieve cached texture after loading" };
}

auto TextureManagerHandle::getOrLoadTexture(
    const fs::path &path,
    const std::string &debugName
) -> std::expected<TextureCache::CacheEntry *, std::string>
{
    auto cached = getTexture(path);
    if (cached) {
        return cached.value();
    }

    return loadTexture(path, debugName);
}

void TextureManagerHandle::preloadTextures(const std::vector<fs::path> &paths)
{
    if (!m_config.enablePreloading || paths.empty()) {
        Logger::trace("Preloading disabled or no textures to preload");
        return;
    }

    Logger::info("Preloading {} textures", paths.size());

    for (const auto &path : paths) {
        if (path.empty()) {
            Logger::warning("Skipping empty path in preload");
            continue;
        }

        auto result = loadTexture(path);
        if (!result) {
            if (m_config.logWarnings) {
                Logger::warning("Failed to preload texture: {}", path.string());
            }
        }
    }
}

void TextureManagerHandle::clearCache()
{
    m_cache->cleanup();
    m_loadedTextures.clear();
    m_errorHistory.clear();
    m_lastError.clear();

    Logger::info("Texture cache cleared");
}

auto TextureManagerHandle::getCacheInfo() const -> std::string
{
    const auto &stats = getStats();
    return std::format(
        "Textures: {}/{}, Memory: {:.1f}MB, Hit Rate: {:.1f}%",
        stats.totalTextures,
        m_config.maxTextures,
        static_cast<f32>(stats.totalMemoryUsage) / (1024 * 1024),
        stats.hitRate * 100.0F
    );
}

auto TextureManagerHandle::createPlaceholderTexture() -> Texture
{
    TextureCreateInfo placeholderInfo;
    placeholderInfo.width = 1;
    placeholderInfo.height = 1;
    placeholderInfo.format = ImageFormat::R8G8B8A8_UNORM;
    placeholderInfo.debugName = "PlaceholderTexture";

    std::vector<std::byte> placeholderData = { std::byte{ 255 },
                                               std::byte{ 0 },
                                               std::byte{ 0 },
                                               std::byte{ 255 } };
    placeholderInfo.imageData = std::span<const std::byte>(placeholderData);

    auto result = m_gpu->createTexture(placeholderInfo);
    if (!result) {
        Logger::error(
            "Failed to create placeholder texture: {}",
            result.error()
        );
        return Texture{};
    }

    return std::move(result.value());
}

auto TextureManagerHandle::handleTextureError(
    const fs::path &path,
    const std::string &error
) -> Texture
{
    m_errorHistory[path] = error;
    m_lastError = error;

    Logger::error("Texture error for {}: {}", path.string(), error);

    if (m_config.createPlaceholderOnError) {
        return createPlaceholderTexture();
    }

    return Texture{};
}

auto TextureManagerHandle::getLastError() const -> std::string
{
    return m_lastError;
}

void TextureManagerHandle::clearErrors()
{
    m_lastError.clear();
    m_errorHistory.clear();
}

auto TextureManagerHandle::getBindlessIndex(const fs::path &path)
    -> std::expected<u32, std::string>
{
    auto entry = getTexture(path);
    if (!entry) {
        return std::unexpected{ entry.error() };
    }

    return entry.value()->bindlessIndex;
}

} // namespace vostok::graphics