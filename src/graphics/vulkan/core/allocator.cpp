#include "graphics/vulkan/core/allocator.hpp"

#include "core/logger/logger.hpp"
#include "graphics/vulkan/utils/vk_utils.hpp"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include <vk_mem_alloc.h>
#include <volk.h>

namespace vostok::graphics::vulkan
{

Allocator::Allocator(VmaAllocator allocator)
    : m_allocator(allocator)
{
    Logger::info("VMA allocator created");
}

Allocator::~Allocator()
{
    vmaDestroyAllocator(m_allocator);
}

Allocator::Allocator(Allocator &&other) noexcept
    : m_allocator(other.m_allocator)
{
    other.m_allocator = VK_NULL_HANDLE;
}

auto Allocator::operator=(Allocator &&other) noexcept -> Allocator &
{
    if (this != &other) {
        m_allocator = other.m_allocator;
        other.m_allocator = VK_NULL_HANDLE;
    }

    return *this;
}

auto Allocator::create(const CreateInfo &createInfo)
    -> std::expected<std::unique_ptr<Allocator>, std::string>
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
            "Failed to create VMA allocator : " + utils::vkResultToString(result)
        );
    }

    auto allocatorPtr = std::unique_ptr<Allocator>(new Allocator(allocator));

    return allocatorPtr;
}

} // namespace vostok::graphics::vulkan