#include "graphics/backends/vulkan/core/vulkan_allocator.hpp"

#include "core/logger/logger.hpp"
#include "graphics/backends/vulkan/utils/vk_utils.hpp"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_VULKAN_VERSION 1004000

#include <vk_mem_alloc.h>
#include <volk.h>

namespace vostok::graphics::vulkan
{

VulkanAllocator::VulkanAllocator(VmaAllocator allocator)
    : m_allocator(allocator)
{
    Logger::info("VMA allocator created");
}

VulkanAllocator::~VulkanAllocator()
{
    vmaDestroyAllocator(m_allocator);
}

VulkanAllocator::VulkanAllocator(VulkanAllocator &&other) noexcept
    : m_allocator(other.m_allocator)
{
    other.m_allocator = VK_NULL_HANDLE;
}

auto VulkanAllocator::operator=(VulkanAllocator &&other) noexcept
    -> VulkanAllocator &
{
    if (this != &other) {
        m_allocator = other.m_allocator;
        other.m_allocator = VK_NULL_HANDLE;
    }

    return *this;
}

auto VulkanAllocator::create(const CreateInfo &createInfo)
    -> std::expected<std::unique_ptr<VulkanAllocator>, std::string>
{
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo vmaCreateInfo = {};
    vmaCreateInfo.vulkanApiVersion = createInfo.vulkanApiVersion;
    vmaCreateInfo.physicalDevice = createInfo.physicalDevice->getHandle();
    vmaCreateInfo.device = createInfo.device->getHandle();
    vmaCreateInfo.instance = createInfo.instance->getHandle();
    vmaCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VmaAllocator allocator = VK_NULL_HANDLE;

    VkResult result = vmaCreateAllocator(&vmaCreateInfo, &allocator);
    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to create VMA allocator : " +
            utils::vkResultToString(result)
        );
    }

    auto allocatorPtr =
        std::unique_ptr<VulkanAllocator>(new VulkanAllocator(allocator));

    return allocatorPtr;
}

auto VulkanAllocator::mapMemory(VmaAllocation allocation, void **data)
    -> std::expected<void *, std::string>
{
    VkResult result = vmaMapMemory(m_allocator, allocation, data);
    if (result != VK_SUCCESS) {
        return std::unexpected(utils::vkResultToString(result));
    }

    return *data;
}

void VulkanAllocator::unmapMemory(VmaAllocation allocation)
{
    vmaUnmapMemory(m_allocator, allocation);
}

auto VulkanAllocator::createBuffer(
    const VkBufferCreateInfo &createInfo,
    VmaMemoryUsage memoryUsage
) -> std::expected<std::pair<VkBuffer, VmaAllocation>, std::string>
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.usage = memoryUsage;

    VkResult result = vmaCreateBuffer(
        m_allocator,
        &createInfo,
        &allocationCreateInfo,
        &buffer,
        &allocation,
        &allocationInfo
    );

    if (result != VK_SUCCESS) {
        return std::unexpected(utils::vkResultToString(result));
    }

    return std::make_pair(buffer, allocation);
}

void VulkanAllocator::destroyBuffer(VkBuffer buffer, VmaAllocation allocation)
{
    vmaDestroyBuffer(m_allocator, buffer, allocation);
}

} // namespace vostok::graphics::vulkan