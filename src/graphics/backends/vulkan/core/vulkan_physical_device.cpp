#include "graphics/backends/vulkan/core/vulkan_physical_device.hpp"

#include "core/logger/logger.hpp"
#include "graphics/backends/vulkan/utils/vk_utils.hpp"
#include "utils/stl/optional.inl"

#include <algorithm>
#include <cstring>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan.h>

namespace vu = vostok::utils;

namespace vostok::graphics::vulkan
{

VulkanPhysicalDevice::VulkanPhysicalDevice(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface
)
    : m_physicalDevice(physicalDevice)
{
    Logger::debug("Initializing physical device");

    queryPhysicalDeviceFeatures();
    queryPhysicalDeviceProperties();
    determineAvailableExtensions();
    initializeQueueFamilyIndices(surface);

    Logger::info("Selected GPU: {}", m_properties.deviceName);
    Logger::debug(
        "GPU type: {}",
        utils::physicalDeviceTypeToString(m_properties.deviceType)
    );
    Logger::debug(
        "API Version: {}.{}.{}",
        VK_VERSION_MAJOR(m_properties.apiVersion),
        VK_VERSION_MINOR(m_properties.apiVersion),
        VK_VERSION_PATCH(m_properties.apiVersion)
    );
}

VulkanPhysicalDevice::~VulkanPhysicalDevice()
{
    m_physicalDevice = VK_NULL_HANDLE;
}

VulkanPhysicalDevice::VulkanPhysicalDevice(
    VulkanPhysicalDevice &&other
) noexcept
    : m_physicalDevice(other.m_physicalDevice),
      m_properties(other.m_properties),
      m_memoryProperties(other.m_memoryProperties),
      m_features(other.m_features),
      m_supportedFeatures(other.m_supportedFeatures),
      m_queueFamilyIndices(other.m_queueFamilyIndices),
      m_supportedExtensions(std::move(other.m_supportedExtensions))
{
    other.m_physicalDevice = VK_NULL_HANDLE;
}

auto VulkanPhysicalDevice::operator=(VulkanPhysicalDevice &&other) noexcept
    -> VulkanPhysicalDevice &
{
    if (this != &other) {
        m_physicalDevice = other.m_physicalDevice;
        m_properties = other.m_properties;
        m_memoryProperties = other.m_memoryProperties;
        m_features = other.m_features;
        m_supportedFeatures = other.m_supportedFeatures;
        m_queueFamilyIndices = other.m_queueFamilyIndices;
        m_supportedExtensions = std::move(other.m_supportedExtensions);

        other.m_physicalDevice = VK_NULL_HANDLE;
    }
    return *this;
}

auto VulkanPhysicalDevice::create(VkInstance instance, VkSurfaceKHR surface)
    -> std::expected<std::unique_ptr<VulkanPhysicalDevice>, std::string>
{
    Logger::info("try to creating Vulkan physical device");

    u32 deviceCount = 0;
    VkResult result =
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to enumerate physical devices: " +
            utils::vkResultToString(result)
        );
    }

    if (deviceCount == 0) {
        return std::unexpected("No physical devices found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to enumerate physical devices: " +
            utils::vkResultToString(result)
        );
    }

    Logger::info("Found {} Vulkan-compatible GPU(s)", deviceCount);

    auto deviceRankingFunc = [surface](VkPhysicalDevice device) -> int {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            device,
            &queueFamilyCount,
            nullptr
        );
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
            device,
            &queueFamilyCount,
            queueFamilies.data()
        );

        bool hasGraphicsQueue = false;
        bool hasPresentQueue = false;

        for (u32 i = 0; i < queueFamilyCount; ++i) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                hasGraphicsQueue = true;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                device,
                i,
                surface,
                &presentSupport
            );
            if (presentSupport) {
                hasPresentQueue = true;
            }
        }

        if (!hasGraphicsQueue && !hasPresentQueue) {
            return 0;
        }

        int score = 1;

        if (deviceProperties.deviceType ==
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        } else if (deviceProperties.deviceType ==
                   VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            score += 100;
        }

        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

        VkDeviceSize totalMemory = 0;
        for (u32 i = 0; i < memoryProperties.memoryHeapCount; i++) {
            if (memoryProperties.memoryHeaps[i].flags &
                VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                totalMemory += memoryProperties.memoryHeaps[i].size;
            }
        }

        constexpr VkDeviceSize BYTES_PER_GB = 1024ULL * 1024ULL * 1024ULL;
        score += static_cast<int>(totalMemory / BYTES_PER_GB);

        return score;
    };

    std::ranges::sort(
        devices.begin(),
        devices.end(),
        [&deviceRankingFunc](VkPhysicalDevice a, VkPhysicalDevice b) {
            return deviceRankingFunc(a) > deviceRankingFunc(b);
        }
    );

    for (const auto &device : devices) {
        int score = deviceRankingFunc(device);
        if (score > 0) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            Logger::info(
                "Selected GPU: {} (Score: {})",
                props.deviceName,
                score
            );

            auto physicalDevice = std::unique_ptr<VulkanPhysicalDevice>(
                new VulkanPhysicalDevice(device, surface)
            );

            return physicalDevice;
        }
    }

    return std::unexpected(
        "Failed to find a suitable GPU. No GPU meets the requirements."
    );
}

void VulkanPhysicalDevice::queryPhysicalDeviceProperties()
{
    vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memoryProperties);
}

void VulkanPhysicalDevice::queryPhysicalDeviceFeatures()
{
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_features);
    m_supportedFeatures.geometryShader = m_features.geometryShader == VK_TRUE;
}

void VulkanPhysicalDevice::determineAvailableExtensions()
{
    u32 extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(
        m_physicalDevice,
        nullptr,
        &extensionCount,
        nullptr
    );

    m_supportedExtensions.resize(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        m_physicalDevice,
        nullptr,
        &extensionCount,
        m_supportedExtensions.data()
    );

    Logger::debug("Device supports {} extensions", extensionCount);

    for (const auto &extension : m_supportedExtensions) {
        Logger::trace("  {}", extension.extensionName);
    }
}

void VulkanPhysicalDevice::initializeQueueFamilyIndices(VkSurfaceKHR surface)
{
    u32 queueFammilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        m_physicalDevice,
        &queueFammilyCount,
        nullptr
    );

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFammilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        m_physicalDevice,
        &queueFammilyCount,
        queueFamilies.data()
    );

    for (u32 i = 0; i < queueFammilyCount; i++) {
        const auto &queueFamily = queueFamilies[i];

        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 &&
            !m_queueFamilyIndices.graphicsFamily.has_value()) {
            m_queueFamilyIndices.graphicsFamily = i;
        }

        if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 &&
            !m_queueFamilyIndices.computeFamily.has_value()) {
            m_queueFamilyIndices.computeFamily = i;
        }

        if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 &&
            !m_queueFamilyIndices.transferFamily.has_value()) {
            m_queueFamilyIndices.transferFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            m_physicalDevice,
            i,
            surface,
            &presentSupport
        );

        if (presentSupport == VK_TRUE &&
            !m_queueFamilyIndices.presentFamily.has_value()) {
            m_queueFamilyIndices.presentFamily = i;
        }
    }
    if (m_queueFamilyIndices.isComplete()) {
        Logger::debug(
            "Queue Families: graphics={}, present={}, compute={}, transfer={}",
            vu::getValueSafe(m_queueFamilyIndices.graphicsFamily, "graphics"),
            vu::getValueSafe(m_queueFamilyIndices.presentFamily, "present"),
            vu::getValueSafe(m_queueFamilyIndices.computeFamily, "compute"),
            vu::getValueSafe(m_queueFamilyIndices.transferFamily, "transfer")
        );
    } else {
        Logger::warning(
            "Not all required queue families are available for this device"
        );
    }
}

auto VulkanPhysicalDevice::isFormatSupported(
    VkFormat format,
    VkImageTiling tiling,
    VkFormatFeatureFlags features
) const -> bool
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR) {
        return (props.linearTilingFeatures & features) == features;
    }

    if (tiling == VK_IMAGE_TILING_OPTIMAL) {
        return (props.optimalTilingFeatures & features) == features;
    }

    return false;
}

auto VulkanPhysicalDevice::checkDeviceExtensionSupport(
    const std::vector<const char *> &requiredExtensions
) const -> bool
{
    std::vector<std::string> requiredExtensionsStr(
        requiredExtensions.begin(),
        requiredExtensions.end()
    );

    for (const auto &requiredExt : requiredExtensionsStr) {
        bool found = false;

        for (const auto &extension : m_supportedExtensions) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            if (strcmp(extension.extensionName, requiredExt.c_str()) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

auto VulkanPhysicalDevice::supportsBindlessResources() const -> bool
{
    return true;
}

auto VulkanPhysicalDevice::getMaxDescriptorSets() const -> u32
{
    return 1024;
}

} // namespace vostok::graphics::vulkan