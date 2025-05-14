#pragma once

#include "vostok/graphics/vulkan/core/physical_device.hpp"

#include <expected>
#include <vulkan/vulkan_core.h>

namespace vostok::graphics::vulkan
{

class Device
{
public:
    struct CreateInfo
    {
        PhysicalDevice *physicalDevice = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        std::vector<const char *> extensions;
        bool enableValidationLayers = false;
    };

    static std::expected<std::unique_ptr<Device>, std::string> create(const CreateInfo &createInfo);

    ~Device();
    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;
    Device(Device &&other) noexcept;
    Device &operator=(Device &&other) noexcept;

    [[nodiscard]] VkDevice getHandle() const { return m_device; }
    [[nodiscard]] PhysicalDevice *getPhysicalDevice() const { return m_physicalDevice; }
    [[nodiscard]] VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    [[nodiscard]] VkQueue getPresentQueue() const { return m_presentQueue; }
    [[nodiscard]] VkQueue getComputeQueue() const { return m_computeQueue; }
    [[nodiscard]] VkQueue getTransferQueue() const { return m_transferQueue; }
    [[nodiscard]] VkCommandPool getCommandPool() const { return m_commandPool; }

    void waitIdle() const;

private:
    Device(VkDevice device, PhysicalDevice *physicalDevice);

    bool initQueues();
    bool createCommandPool();

    VkDevice m_device = VK_NULL_HANDLE;
    PhysicalDevice *m_physicalDevice = nullptr;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan