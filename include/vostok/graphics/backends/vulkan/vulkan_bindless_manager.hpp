#pragma once

#include "vostok/core/type.hpp"
#include "vostok/graphics/buffer.hpp"
#include "vostok/graphics/buffers/bindable_resource.hpp"

#include <expected>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanInstance;
class VulkanDevice;
class VulkanFrameSync;
class VulkanAllocator;

struct VulkanBindlessManagerCreateInfo
{
    VulkanInstance *instance = nullptr;
    VulkanDevice *device = nullptr;
    VulkanFrameSync *frameSync = nullptr;
    VulkanAllocator *allocator = nullptr;
    u32 maxUBOs = 1024;
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

    auto registerUBO(BindableResource *ubo, size_t size)
        -> std::expected<u32, std::string>;

    auto unregisterUBO(const BindableResource *ubo)
        -> std::expected<void, std::string>;

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

    auto updateUBO(u32 index, const BindableResource *resource)
        -> std::expected<void, std::string>;

    auto createGPUBuffer(size_t size, const void *data)
        -> std::expected<std::unique_ptr<Buffer>, std::string>;

    auto updateDescriptorSet(u32 index, VkBuffer buffer, size_t size)
        -> std::expected<void, std::string>;

    VulkanInstance *m_instance;
    VulkanDevice *m_device;
    VulkanFrameSync *m_frameSync;
    VulkanAllocator *m_allocator;

    u32 m_maxUBOs;

    std::unordered_map<const BindableResource *, u32> m_uboToIndex;
    std::unordered_map<u32, const BindableResource *> m_indexToUBO;

    std::vector<const BindableResource *> m_dirtyStack;
    mutable std::mutex m_dirtyMutex;

    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    std::vector<std::unique_ptr<Buffer>> m_gpuBuffers;
    std::vector<size_t> m_bufferSizes;
};

} // namespace vostok::graphics::vulkan