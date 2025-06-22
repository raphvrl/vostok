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
        void *pNext = nullptr;
    };

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<Device>, std::string>;

    ~Device();
    Device(const Device &) = delete;
    auto operator=(const Device &) -> Device & = delete;
    Device(Device &&other) noexcept;
    auto operator=(Device &&other) noexcept -> Device &;

    [[nodiscard]] auto getHandle() const -> VkDevice { return m_device; }
    [[nodiscard]] auto getPhysicalDevice() const -> PhysicalDevice * { return m_physicalDevice; }
    [[nodiscard]] auto getGraphicsQueue() const -> VkQueue { return m_graphicsQueue; }
    [[nodiscard]] auto getPresentQueue() const -> VkQueue { return m_presentQueue; }
    [[nodiscard]] auto getComputeQueue() const -> VkQueue { return m_computeQueue; }
    [[nodiscard]] auto getTransferQueue() const -> VkQueue { return m_transferQueue; }
    [[nodiscard]] auto getCommandPool() const -> VkCommandPool { return m_commandPool; }

    void waitIdle() const;

private:
    Device(VkDevice device, PhysicalDevice *physicalDevice);

    auto initQueues() -> bool;
    auto createCommandPool() -> bool;

    VkDevice m_device = VK_NULL_HANDLE;
    PhysicalDevice *m_physicalDevice = nullptr;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan