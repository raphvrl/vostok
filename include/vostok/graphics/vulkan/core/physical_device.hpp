#pragma once

#include "vostok/core/type.hpp"

#include <cassert>
#include <expected>
#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace vostok::graphics::vulkan
{

struct QueueFamilyIndices
{
    std::optional<u32> graphicsFamily;
    std::optional<u32> presentFamily;
    std::optional<u32> computeFamily;
    std::optional<u32> transferFamily;

    [[nodiscard]] bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value() &&
               computeFamily.has_value() && transferFamily.has_value();
    }
};

class PhysicalDevice
{
public:
    struct Features
    {
        bool geometryShader = false;
    };

    static std::expected<std::unique_ptr<PhysicalDevice>, std::string>
    create(VkInstance instance, VkSurfaceKHR surface);

    ~PhysicalDevice();

    PhysicalDevice(const PhysicalDevice &) = delete;
    PhysicalDevice &operator=(const PhysicalDevice &) = delete;
    PhysicalDevice(PhysicalDevice &&other) noexcept;
    PhysicalDevice &operator=(PhysicalDevice &&other) noexcept;

    [[nodiscard]] VkPhysicalDevice getHandle() const { return m_physicalDevice; }
    [[nodiscard]] const VkPhysicalDeviceProperties &getProperties() const { return m_properties; }
    [[nodiscard]] const VkPhysicalDeviceMemoryProperties &getMemoryProperties() const
    {
        return m_memoryProperties;
    }

    [[nodiscard]] const Features &getSupportedFeatures() const { return m_supportedFeatures; }
    [[nodiscard]] const QueueFamilyIndices &getQueueFamilyIndices() const
    {
        return m_queueFamilyIndices;
    }

    [[nodiscard]] bool
    isFormatSupported(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    [[nodiscard]] bool
    checkDeviceExtensionSupport(const std::vector<const char *> &requiredExtensions) const;

private:
    PhysicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

    void initializeQueueFamilyIndices(VkSurfaceKHR surface);
    void queryPhysicalDeviceProperties();
    void queryPhysicalDeviceFeatures();
    void determineAvailableExtensions();

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties m_properties = {};
    VkPhysicalDeviceMemoryProperties m_memoryProperties = {};
    VkPhysicalDeviceFeatures m_features = {};
    Features m_supportedFeatures = {};
    QueueFamilyIndices m_queueFamilyIndices = {};
    std::vector<VkExtensionProperties> m_supportedExtensions;
};

} // namespace vostok::graphics::vulkan