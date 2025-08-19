#pragma once

#include "vostok/core/type.hpp"
#include "vostok/graphics/textures/texture.hpp"

#include <chrono>
#include <filesystem>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

namespace vostok::graphics
{

class GPUHandle;

struct TextureCacheConfig
{
    size_t maxMemoryBytes = static_cast<size_t>(1024 * 1024 * 1024);
    size_t maxTextures = 1000;
    bool enableLRU = true;
    bool enableCompression = false;
    bool enableAsyncEviction = true;

    float evictionThreshold = 0.8F;
    size_t minEvictionSize = static_cast<size_t>(1024 * 1024);
};

struct TextureCacheEntry
{
    Texture texture;
    fs::path filePath;
    size_t memoryUsage;
    std::chrono::steady_clock::time_point lastAccess;
    std::chrono::steady_clock::time_point creationTime;
    u32 accessCount = 0;
    bool isPinned = false;
    std::string debugName;

    u32 width = 0;
    u32 height = 0;
    ImageFormat format = ImageFormat::UNDEFINED;

    u32 referenceCount = 0;
};

struct TextureCacheStats
{
    size_t totalMemoryUsage = 0;
    size_t totalTextures = 0;
    size_t cacheHits = 0;
    size_t cacheMisses = 0;
    size_t evictions = 0;
    float hitRate = 0.0F;

    std::chrono::microseconds averageLoadTime{ 0 };
    std::chrono::microseconds averageAccessTime{ 0 };
};

class TextureCache
{
public:
    using CacheConfig = TextureCacheConfig;
    using CacheEntry = TextureCacheEntry;
    using CacheStats = TextureCacheStats;

    static auto create(GPUHandle *gpu, const CacheConfig &config)
        -> std::expected<std::unique_ptr<TextureCache>, std::string>;

    ~TextureCache() = default;

    TextureCache(const TextureCache &) = delete;
    TextureCache(TextureCache &&) = delete;
    auto operator=(const TextureCache &) -> TextureCache & = delete;
    auto operator=(TextureCache &&) -> TextureCache & = delete;

    [[nodiscard]] auto get(const fs::path &path) -> std::optional<CacheEntry *>;
    [[nodiscard]] auto
    put(const fs::path &path,
        Texture &&texture,
        const std::string &debugName = "") -> bool;
    void remove(const fs::path &path);

    auto evictLRU(size_t requiredMemory = 0) -> size_t;
    auto evictOldest(size_t requiredMemory = 0) -> size_t;
    auto evictByMemoryUsage(size_t targetMemory) -> size_t;

    void pinTexture(const fs::path &path);
    void unpinTexture(const fs::path &path);

    auto incrementReference(const fs::path &path) -> size_t;
    auto decrementReference(const fs::path &path) -> size_t;

    [[nodiscard]] auto getMemoryUsage() const -> size_t;
    [[nodiscard]] auto getTextureCount() const -> size_t;
    [[nodiscard]] auto getStats() const -> const CacheStats &;
    [[nodiscard]] auto getStatsWithHitRate() const -> CacheStats;
    void resetStats();

    void update();
    void cleanup();
    void preloadTextures(const std::vector<fs::path> &paths);

private:
    using LRUList = std::list<fs::path>;
    using LRUIterator = LRUList::iterator;
    using PathToEntry = std::unordered_map<fs::path, CacheEntry>;
    using PathToLRU = std::unordered_map<fs::path, LRUIterator>;

    explicit TextureCache(GPUHandle *gpu, const CacheConfig &config);

    GPUHandle *m_gpu = nullptr;

    CacheConfig m_config;
    PathToEntry m_entries;
    PathToLRU m_lruMap;
    LRUList m_lruList;

    mutable std::mutex m_mutex;

    CacheStats m_stats;
    std::chrono::steady_clock::time_point m_lastStatsUpdate;

    void updateLRU(const fs::path &path);
    void evictEntry(const fs::path &path);
    void updateStats();
    static auto calculateMemoryUsage(const Texture &texture) -> size_t;
    void logCacheOperation(
        const std::string &operation,
        const fs::path &path,
        size_t memoryUsage
    );
};

} // namespace vostok::graphics