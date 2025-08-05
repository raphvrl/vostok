#include "graphics/backends/vulkan/core/vulkan_command_pool.hpp"

#include "core/logger/logger.hpp"
#include "graphics/backends/vulkan/core/vulkan_device.hpp"
#include "graphics/backends/vulkan/utils/vk_utils.hpp"

#include <volk.h>

namespace vostok::graphics::vulkan
{

auto VulkanCommandPool::create(const CreateInfo &createInfo)
    -> std::expected<std::unique_ptr<VulkanCommandPool>, std::string>
{
    auto *device = createInfo.device;
    if (createInfo.device == nullptr) {
        return std::unexpected("Device is null");
    }

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = createInfo.flags;
    poolInfo.queueFamilyIndex = createInfo.queueFamilyIndex;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkResult result = vkCreateCommandPool(
        device->getHandle(),
        &poolInfo,
        nullptr,
        &commandPool
    );

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to create command pool: " + utils::vkResultToString(result)
        );
    }

    auto commandPoolPtr = std::unique_ptr<VulkanCommandPool>(
        new VulkanCommandPool(device, commandPool)
    );

    Logger::debug(
        "Command pool created successfully for queue family {}",
        createInfo.queueFamilyIndex
    );

    return commandPoolPtr;
}

VulkanCommandPool::VulkanCommandPool(VulkanDevice *device, VkCommandPool pool)
    : m_device(device),
      m_pool(pool)
{}

VulkanCommandPool::~VulkanCommandPool()
{
    if (m_pool != VK_NULL_HANDLE && m_device != nullptr) {
        Logger::debug("Destroying command pool for queue family");
        vkDestroyCommandPool(m_device->getHandle(), m_pool, nullptr);
        Logger::debug("Command pool destroyed successfully");
    } else {
        Logger::warning(
            "Command pool destructor called with invalid state: pool={}, "
            "device={}",
            m_pool != VK_NULL_HANDLE,
            m_device != nullptr
        );
    }
}

VulkanCommandPool::VulkanCommandPool(VulkanCommandPool &&other) noexcept
    : m_device(other.m_device),
      m_pool(other.m_pool)
{
    other.m_device = nullptr;
    other.m_pool = VK_NULL_HANDLE;
}

auto VulkanCommandPool::operator=(VulkanCommandPool &&other) noexcept
    -> VulkanCommandPool &
{
    if (this != &other) {
        if (m_pool != VK_NULL_HANDLE && m_device != nullptr) {
            vkDestroyCommandPool(m_device->getHandle(), m_pool, nullptr);
        }

        m_device = other.m_device;
        m_pool = other.m_pool;

        other.m_device = nullptr;
        other.m_pool = VK_NULL_HANDLE;
    }

    return *this;
}

auto VulkanCommandPool::allocate(VkCommandBufferLevel level)
    -> std::expected<VkCommandBuffer, std::string>
{
    if (m_pool == VK_NULL_HANDLE) {
        return std::unexpected("Command pool is not initialized");
    }

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_pool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkResult result = vkAllocateCommandBuffers(
        m_device->getHandle(),
        &allocInfo,
        &commandBuffer
    );

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to allocate command buffer: " +
            utils::vkResultToString(result)
        );
    }

    Logger::debug("Command buffer allocated successfully");
    return commandBuffer;
}

void VulkanCommandPool::free(VkCommandBuffer buffer)
{
    if (m_pool != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(m_device->getHandle(), m_pool, 1, &buffer);
        Logger::debug("Command buffer freed");
    }
}

void VulkanCommandPool::free(std::span<VkCommandBuffer> buffers)
{
    if (m_pool != VK_NULL_HANDLE && !buffers.empty()) {
        vkFreeCommandBuffers(
            m_device->getHandle(),
            m_pool,
            static_cast<u32>(buffers.size()),
            buffers.data()
        );

        Logger::debug("{} command buffers freed", buffers.size());
    }
}

void VulkanCommandPool::reset() const
{
    if (m_pool != VK_NULL_HANDLE) {
        VkResult result = vkResetCommandPool(m_device->getHandle(), m_pool, 0);
        if (result != VK_SUCCESS) {
            Logger::warning(
                "Failed to reset command pool: {}",
                utils::vkResultToString(result)
            );
        } else {
            Logger::debug("Command pool reset successfully");
        }
    }
}

} // namespace vostok::graphics::vulkan