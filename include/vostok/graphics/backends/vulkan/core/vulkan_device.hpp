#pragma once

#include "vostok/graphics/backends/vulkan/core/vulkan_physical_device.hpp"

#include <expected>
#include <vulkan/vulkan_core.h>

namespace vostok::graphics::vulkan
{

class VulkanDevice
{
public:
    struct CreateInfo
    {
        VulkanPhysicalDevice *physicalDevice = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        std::vector<const char *> extensions;
        bool enableValidationLayers = false;
        void *pNext = nullptr;
    };

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<VulkanDevice>, std::string>;

    ~VulkanDevice();
    VulkanDevice(const VulkanDevice &) = delete;
    auto operator=(const VulkanDevice &) -> VulkanDevice & = delete;
    VulkanDevice(VulkanDevice &&other) noexcept;
    auto operator=(VulkanDevice &&other) noexcept -> VulkanDevice &;

    [[nodiscard]] auto getHandle() const -> VkDevice { return m_device; }
    [[nodiscard]] auto getPhysicalDevice() const -> VulkanPhysicalDevice *
    {
        return m_physicalDevice;
    }
    [[nodiscard]] auto getGraphicsQueue() const -> VkQueue
    {
        return m_graphicsQueue;
    }
    [[nodiscard]] auto getPresentQueue() const -> VkQueue
    {
        return m_presentQueue;
    }
    [[nodiscard]] auto getComputeQueue() const -> VkQueue
    {
        return m_computeQueue;
    }
    [[nodiscard]] auto getTransferQueue() const -> VkQueue
    {
        return m_transferQueue;
    }
    [[nodiscard]] auto getCommandPool() const -> VkCommandPool
    {
        return m_commandPool;
    }

    void waitIdle() const;

private:
    VulkanDevice(VkDevice device, VulkanPhysicalDevice *physicalDevice);

    auto initQueues() -> bool;
    auto createCommandPool() -> bool;

    VkDevice m_device = VK_NULL_HANDLE;
    VulkanPhysicalDevice *m_physicalDevice = nullptr;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan