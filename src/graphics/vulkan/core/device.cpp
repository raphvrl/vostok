#include "graphics/vulkan/core/device.hpp"

#include "core/logger/logger.hpp"
#include "graphics/vulkan/utils/vk_utils.hpp"
#include "utils/stl/optional.inl"
#include "volk.h"

#include <expected>
#include <set>
#include <vulkan/vulkan_core.h>

namespace vu = vostok::utils;
namespace vostok::graphics::vulkan
{

Device::Device(VkDevice device, PhysicalDevice *physicalDevice)
    : m_device(device),
      m_physicalDevice(physicalDevice)
{
    Logger::info("Vulkan device created");
}

Device::~Device()
{
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    m_physicalDevice = nullptr;
    Logger::debug("Vulkan device destroyed");
}

Device::Device(Device &&other) noexcept
    : m_device(other.m_device),
      m_physicalDevice(other.m_physicalDevice),
      m_graphicsQueue(other.m_graphicsQueue),
      m_presentQueue(other.m_presentQueue),
      m_computeQueue(other.m_computeQueue),
      m_transferQueue(other.m_transferQueue),
      m_commandPool(other.m_commandPool)
{
    other.m_device = VK_NULL_HANDLE;
    other.m_physicalDevice = nullptr;
    other.m_graphicsQueue = VK_NULL_HANDLE;
    other.m_presentQueue = VK_NULL_HANDLE;
    other.m_computeQueue = VK_NULL_HANDLE;
    other.m_transferQueue = VK_NULL_HANDLE;
    other.m_commandPool = VK_NULL_HANDLE;
}

Device &Device::operator=(Device &&other) noexcept
{
    if (this != &other) {
        this->~Device();

        m_device = other.m_device;
        m_physicalDevice = other.m_physicalDevice;
        m_graphicsQueue = other.m_graphicsQueue;
        m_presentQueue = other.m_presentQueue;
        m_computeQueue = other.m_computeQueue;
        m_transferQueue = other.m_transferQueue;
        m_commandPool = other.m_commandPool;

        other.m_device = VK_NULL_HANDLE;
        other.m_physicalDevice = nullptr;
        other.m_graphicsQueue = VK_NULL_HANDLE;
        other.m_presentQueue = VK_NULL_HANDLE;
        other.m_computeQueue = VK_NULL_HANDLE;
        other.m_transferQueue = VK_NULL_HANDLE;
        other.m_commandPool = VK_NULL_HANDLE;
    }
    return *this;
}

std::expected<std::unique_ptr<Device>, std::string> Device::create(const CreateInfo &createInfo)
{
    if (createInfo.physicalDevice == nullptr) {
        return std::unexpected("Physical device is null is required");
    }

    const QueueFamilyIndices &queueFamilyIndices =
        createInfo.physicalDevice->getQueueFamilyIndices();

    if (!queueFamilyIndices.isComplete()) {
        return std::unexpected("Graphics family indices are not complete");
    }

    std::set<u32> uniqueQueueFamilies = {
        vu::getValueSafe(queueFamilyIndices.graphicsFamily, "graphics"),
        vu::getValueSafe(queueFamilyIndices.presentFamily, "present"),
        vu::getValueSafe(queueFamilyIndices.computeFamily, "compute"),
        vu::getValueSafe(queueFamilyIndices.transferFamily, "transfer"),
    };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    f32 queuePriority = 1.0F;

    for (u32 queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    std::vector<const char *> enableLayers;
    if (createInfo.enableValidationLayers) {
        enableLayers.push_back("VK_LAYER_KHRONOS_validation");
        Logger::debug("Vulkan validation layers enabled for device");
    }

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    deviceCreateInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

    deviceCreateInfo.pNext = createInfo.pNext;

    if (createInfo.enableValidationLayers) {
        deviceCreateInfo.enabledLayerCount = static_cast<u32>(enableLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = enableLayers.data();
    } else {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    if (!createInfo.extensions.empty()) {
        deviceCreateInfo.enabledExtensionCount = static_cast<u32>(createInfo.extensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = createInfo.extensions.data();
    } else {
        deviceCreateInfo.enabledExtensionCount = 0;
    }

    if (!createInfo.physicalDevice->checkDeviceExtensionSupport(
            {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            }
        )) {
        return std::unexpected("Device does not support swap chain extension");
    }

    VkDevice device = VK_NULL_HANDLE;
    VkResult result =
        vkCreateDevice(createInfo.physicalDevice->getHandle(), &deviceCreateInfo, nullptr, &device);

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to create Vulkan device: " + utils::vkResultToString(result)
        );
    }

    volkLoadDevice(device);

    auto devicePtr = std::unique_ptr<Device>(new Device(device, createInfo.physicalDevice));

    if (!devicePtr->initQueues()) {
        return std::unexpected("Failed to initialize Vulkan device queues");
    }

    if (!devicePtr->createCommandPool()) {
        return std::unexpected("Failed to create Vulkan command pool");
    }

    return devicePtr;
}

bool Device::initQueues()
{
    const QueueFamilyIndices &indices = m_physicalDevice->getQueueFamilyIndices();

    if (!indices.isComplete()) {
        Logger::error("Queue family indices are not complete");
        return false;
    }

    u32 graphicsFamily = vu::getValueSafe(indices.graphicsFamily, "graphics");
    u32 presentFamily = vu::getValueSafe(indices.presentFamily, "present");
    u32 computeFamily = vu::getValueSafe(indices.computeFamily, "compute");
    u32 transferFamily = vu::getValueSafe(indices.transferFamily, "transfer");

    vkGetDeviceQueue(m_device, graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, presentFamily, 0, &m_presentQueue);
    vkGetDeviceQueue(m_device, computeFamily, 0, &m_computeQueue);
    vkGetDeviceQueue(m_device, transferFamily, 0, &m_transferQueue);

    if (m_graphicsQueue == VK_NULL_HANDLE || m_presentQueue == VK_NULL_HANDLE ||
        m_computeQueue == VK_NULL_HANDLE || m_transferQueue == VK_NULL_HANDLE) {
        Logger::error("Failed to get Vulkan device queues");
        return false;
    }

    Logger::debug("Device queues initialized successfully");
    return true;
}

bool Device::createCommandPool()
{
    const QueueFamilyIndices &indices = m_physicalDevice->getQueueFamilyIndices();

    if (!indices.isComplete()) {
        Logger::error("Queue family indices are not complete");
        return false;
    }

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vu::getValueSafe(indices.graphicsFamily, "graphics");
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        Logger::error("Failed to create command pool: {}", utils::vkResultToString(result));
        return false;
    }

    Logger::debug("Command pool created successfully");
    return true;
}

void Device::waitIdle() const
{
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }
}

}; // namespace vostok::graphics::vulkan