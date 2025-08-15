#pragma once

#include "vostok/graphics/backends/vulkan/core/vulkan_device.hpp"
#include "vostok/graphics/backends/vulkan/core/vulkan_instance.hpp"
#include "vostok/graphics/backends/vulkan/core/vulkan_physical_device.hpp"

#include <expected>
#include <memory>
#include <string>
#include <vk_mem_alloc.h>

namespace vostok::graphics::vulkan
{

class VulkanAllocator
{
public:
    struct CreateInfo
    {
        VulkanInstance *instance = nullptr;
        VulkanPhysicalDevice *physicalDevice = nullptr;
        VulkanDevice *device = nullptr;
        u32 vulkanApiVersion = VK_API_VERSION_1_4;
    };

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<VulkanAllocator>, std::string>;

    ~VulkanAllocator();

    VulkanAllocator(const VulkanAllocator &) = delete;
    auto operator=(const VulkanAllocator &) -> VulkanAllocator & = delete;
    VulkanAllocator(VulkanAllocator &&other) noexcept;
    auto operator=(VulkanAllocator &&other) noexcept -> VulkanAllocator &;

    auto mapMemory(VmaAllocation allocation, void **data)
        -> std::expected<void *, std::string>;
    void unmapMemory(VmaAllocation allocation);

    auto createBuffer(
        const VkBufferCreateInfo &createInfo,
        VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY
    ) -> std::expected<std::pair<VkBuffer, VmaAllocation>, std::string>;

    void destroyBuffer(VkBuffer buffer, VmaAllocation allocation);

    auto createImage(
        const VkImageCreateInfo &createInfo,
        VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY
    ) -> std::expected<std::pair<VkImage, VmaAllocation>, std::string>;

    void destroyImage(VkImage image, VmaAllocation allocation);

    [[nodiscard]] auto getHandle() const -> VmaAllocator { return m_allocator; }

private:
    VulkanAllocator(VmaAllocator allocator);

    VmaAllocator m_allocator = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan