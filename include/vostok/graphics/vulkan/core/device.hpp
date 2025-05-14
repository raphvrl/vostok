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