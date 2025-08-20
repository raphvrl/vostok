#pragma once

#include "vostok/core/type.hpp"
#include "vostok/graphics/buffers/bindable_resource.hpp"
#include "vostok/graphics/buffers/buffer.hpp"
#include "vostok/graphics/textures/texture.hpp"

#include <expected>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

enum class ResourceType : u8
{
    UBO,
    SSBO,
    TEXTURE,
    SAMPLER
};

class VulkanInstance;
class VulkanDevice;
class VulkanFrameSync;
class VulkanAllocator;
class VulkanImage;
class VulkanSampler;

struct VulkanBindlessManagerCreateInfo
{
    VulkanInstance *instance = nullptr;
    VulkanDevice *device = nullptr;
    VulkanFrameSync *frameSync = nullptr;
    VulkanAllocator *allocator = nullptr;
    u32 maxUBOs = 1024;
    u32 maxSSBOs = 1024;
    u32 maxTextures = 1024;
    std::string debugName = "VulkanBindlessManager";
};

class VulkanBindlessManager
{
public:
    using CreateInfo = VulkanBindlessManagerCreateInfo;

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<VulkanBindlessManager>, std::string>;

    ~VulkanBindlessManager();

    VulkanBindlessManager(const VulkanBindlessManager &) = delete;
    auto operator=(const VulkanBindlessManager &)
        -> VulkanBindlessManager & = delete;
    VulkanBindlessManager(VulkanBindlessManager &&) noexcept;
    auto operator=(VulkanBindlessManager &&) noexcept
        -> VulkanBindlessManager &;

    template <typename... Args>
    auto registerResource(
        ResourceType type,
        graphics::BindableResource *resource,
        Args &&...args
    ) -> std::expected<u32, std::string>
    {
        switch (type) {
            case ResourceType::UBO:
                if constexpr (sizeof...(args) >= 1) {
                    return registerUBO(resource, std::forward<Args>(args)...);
                } else {
                    return std::unexpected{ "UBO requires size parameter" };
                }
            case ResourceType::SSBO:
                if constexpr (sizeof...(args) >= 1) {
                    return registerSSBO(resource, std::forward<Args>(args)...);
                } else {
                    return std::unexpected{ "SSBO requires size parameter" };
                }
            case ResourceType::TEXTURE:
                return registerTexture(resource);
            default:
                return std::unexpected{ "Invalid resource type" };
        }
    }

    auto unregisterResource(
        ResourceType type,
        const graphics::BindableResource *resource
    ) -> std::expected<void, std::string>;

    auto update() -> std::expected<void, std::string>;

    void notifyDirty(u32 bindlessIndex);

    [[nodiscard]] auto getRegisteredUBOCount() const noexcept -> u32
    {
        return static_cast<u32>(m_uboToIndex.size());
    }

    [[nodiscard]] auto getMaxUBOCount() const noexcept -> u32
    {
        return m_maxUBOs;
    }

    [[nodiscard]] auto getDirtyUBOCount() const noexcept -> u32
    {
        std::lock_guard<std::mutex> lock(m_dirtyMutex);
        return static_cast<u32>(m_dirtyStack.size());
    }

    [[nodiscard]] auto getDescriptorSet() const -> VkDescriptorSet
    {
        return m_descriptorSet;
    }

    [[nodiscard]] auto getDescriptorSetLayout() const -> VkDescriptorSetLayout
    {
        return m_descriptorSetLayout;
    }

private:
    explicit VulkanBindlessManager(const CreateInfo &createInfo);

    auto initBindlessResources() -> std::expected<void, std::string>;
    void cleanupBindlessResources();

    auto createGlobalSampler() -> std::expected<void, std::string>;

    auto registerUBO(graphics::BindableResource *ubo, size_t size)
        -> std::expected<u32, std::string>;
    auto registerSSBO(graphics::BindableResource *ssbo, size_t size)
        -> std::expected<u32, std::string>;
    auto registerTexture(graphics::BindableResource *texture)
        -> std::expected<u32, std::string>;

    auto unregisterUBO(const graphics::BindableResource *ubo)
        -> std::expected<void, std::string>;
    auto unregisterSSBO(const graphics::BindableResource *ssbo)
        -> std::expected<void, std::string>;
    auto unregisterTexture(const graphics::BindableResource *texture)
        -> std::expected<void, std::string>;

    auto updateUBO(u32 index, const graphics::BindableResource *resource)
        -> std::expected<void, std::string>;
    auto updateSSBO(u32 index, const graphics::BindableResource *resource)
        -> std::expected<void, std::string>;

    auto
    createGPUBuffer(size_t size, const void *data, graphics::BufferUsage usage)
        -> std::expected<std::unique_ptr<Buffer>, std::string>;
    auto createGPUImage(graphics::TextureImpl *texture)
        -> std::expected<std::unique_ptr<VulkanImage>, std::string>;

    auto updateUBODescriptorSet(u32 index, VkBuffer buffer, size_t size)
        -> std::expected<void, std::string>;
    auto updateSSBODescriptorSet(u32 index, VkBuffer buffer, size_t size)
        -> std::expected<void, std::string>;
    auto updateTextureDescriptorSet(
        u32 index,
        VkImageView imageView,
        VkSampler sampler
    ) -> std::expected<void, std::string>;

    VulkanInstance *m_instance;
    VulkanDevice *m_device;
    VulkanFrameSync *m_frameSync;
    VulkanAllocator *m_allocator;

    u32 m_maxUBOs;
    u32 m_maxSSBOs;
    u32 m_maxTextures;

    std::unordered_map<const graphics::BindableResource *, u32> m_uboToIndex;
    std::unordered_map<u32, const graphics::BindableResource *> m_indexToUBO;

    std::unordered_map<const graphics::BindableResource *, u32> m_ssboToIndex;
    std::unordered_map<u32, const graphics::BindableResource *> m_indexToSSBO;

    std::unordered_map<const graphics::BindableResource *, u32>
        m_textureToIndex;
    std::unordered_map<u32, const graphics::BindableResource *>
        m_indexToTexture;

    std::vector<const graphics::BindableResource *> m_dirtyStack;
    mutable std::mutex m_dirtyMutex;

    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    VkSampler m_globalSampler = VK_NULL_HANDLE;

    std::vector<std::unique_ptr<Buffer>> m_gpuBuffers;
    std::vector<size_t> m_bufferSizes;

    std::vector<std::unique_ptr<Image>> m_gpuImages;

    std::vector<std::unique_ptr<VulkanSampler>> m_gpuSamplers;

    auto createSamplerForTexture(graphics::TextureImpl *texture)
        -> std::expected<std::unique_ptr<VulkanSampler>, std::string>;

    auto getBufferIndex(ResourceType type, u32 relativeIndex) const -> u32;
};

} // namespace vostok::graphics::vulkan