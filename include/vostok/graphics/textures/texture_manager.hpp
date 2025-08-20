#pragma once

#include "vostok/core/type.hpp"
#include "vostok/graphics/textures/texture.hpp"
#include "vostok/graphics/textures/texture_cache.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;

namespace vostok::graphics
{

class GPUHandle;

struct TextureManagerConfig
{
    size_t maxMemoryBytes = static_cast<size_t>(512 * 1024 * 1024); // 512MB
    size_t maxTextures = 100;
    bool enableLRU = true;
    bool enableAsyncEviction = true;
    float evictionThreshold = 0.8F;

    bool autoDetectFormat = true;
    bool forceSRGB = false;

    bool createPlaceholderOnError = true;
    bool logWarnings = true;

    bool enableAsyncLoading = false;
    bool enablePreloading = true;

    std::string debugPrefix = "TextureManager";
};

class TextureManagerHandle
{
public:
    using Config = TextureManagerConfig;

    static auto create(GPUHandle *gpu, const Config &config = Config{})
        -> std::expected<std::unique_ptr<TextureManagerHandle>, std::string>;

    ~TextureManagerHandle() = default;

    TextureManagerHandle(const TextureManagerHandle &) = delete;
    TextureManagerHandle(TextureManagerHandle &&) noexcept = default;
    auto operator=(const TextureManagerHandle &)
        -> TextureManagerHandle & = delete;
    auto operator=(TextureManagerHandle &&) noexcept
        -> TextureManagerHandle & = default;

    [[nodiscard]] auto getTexture(const fs::path &path)
        -> std::expected<TextureCache::CacheEntry *, std::string>;

    [[nodiscard]] auto
    loadTexture(const fs::path &path, const std::string &debugName = "")
        -> std::expected<TextureCache::CacheEntry *, std::string>;

    [[nodiscard]] auto
    getOrLoadTexture(const fs::path &path, const std::string &debugName = "")
        -> std::expected<TextureCache::CacheEntry *, std::string>;

    auto incrementReference(const fs::path &path) -> size_t
    {
        return m_cache->incrementReference(path);
    }

    auto decrementReference(const fs::path &path) -> size_t
    {
        return m_cache->decrementReference(path);
    }

    void preloadTextures(const std::vector<fs::path> &paths);
    void clearCache();

    [[nodiscard]] auto getStats() const -> const TextureCache::CacheStats &
    {
        return m_cache->getStats();
    }

    [[nodiscard]] auto getCacheInfo() const -> std::string;

    void update() { m_cache->update(); }

    [[nodiscard]] auto getLastError() const -> std::string;
    void clearErrors();

    [[nodiscard]] auto getCache() -> TextureCache * { return m_cache.get(); }

    [[nodiscard]] auto getBindlessIndex(const fs::path &path)
        -> std::expected<u32, std::string>;

private:
    explicit TextureManagerHandle(GPUHandle *gpu, const Config &config);

    auto createPlaceholderTexture() -> Texture;
    auto handleTextureError(const fs::path &path, const std::string &error)
        -> Texture;

    GPUHandle *m_gpu;
    Config m_config;
    std::unique_ptr<TextureCache> m_cache;

    std::string m_lastError;
    std::unordered_map<fs::path, std::string> m_errorHistory;

    std::unordered_map<fs::path, Texture> m_loadedTextures;
};

struct TextureManager : std::unique_ptr<TextureManagerHandle>
{
    using Base = std::unique_ptr<TextureManagerHandle>;
    using Base::Base;

    TextureManager() = default;
    ~TextureManager() = default;

    TextureManager(TextureManager &&) = default;
    auto operator=(TextureManager &&) -> TextureManager & = default;
    TextureManager(const TextureManager &) = delete;
    auto operator=(const TextureManager &) -> TextureManager & = delete;
    TextureManager(TextureManagerHandle *handle)
        : Base(handle)
    {}

    explicit TextureManager(std::unique_ptr<TextureManagerHandle> &&ptr)
        : Base(std::move(ptr))
    {}

    using CreateInfo = TextureManagerHandle::Config;

    static auto create(GPUHandle *gpu, const CreateInfo &config = CreateInfo{})
        -> std::expected<TextureManager, std::string>
    {
        auto result = TextureManagerHandle::create(gpu, config);
        if (!result) {
            return std::unexpected(result.error());
        }
        return TextureManager(std::move(result.value()));
    }
};
} // namespace vostok::graphics