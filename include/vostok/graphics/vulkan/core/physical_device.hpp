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

    [[nodiscard]] auto isComplete() const -> bool
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

    static auto create(VkInstance instance, VkSurfaceKHR surface)
        -> std::expected<std::unique_ptr<PhysicalDevice>, std::string>;

    ~PhysicalDevice();

    PhysicalDevice(const PhysicalDevice &) = delete;
    auto operator=(const PhysicalDevice &) -> PhysicalDevice & = delete;
    PhysicalDevice(PhysicalDevice &&other) noexcept;
    auto operator=(PhysicalDevice &&other) noexcept -> PhysicalDevice &;

    [[nodiscard]] auto getHandle() const -> VkPhysicalDevice { return m_physicalDevice; }
    [[nodiscard]] auto getProperties() const -> const VkPhysicalDeviceProperties &
    {
        return m_properties;
    }
    [[nodiscard]] auto getMemoryProperties() const -> const VkPhysicalDeviceMemoryProperties &
    {
        return m_memoryProperties;
    }

    [[nodiscard]] auto getSupportedFeatures() const -> const Features &
    {
        return m_supportedFeatures;
    }
    [[nodiscard]] auto getQueueFamilyIndices() const -> const QueueFamilyIndices &
    {
        return m_queueFamilyIndices;
    }

    [[nodiscard]] auto
    isFormatSupported(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features) const
        -> bool;

    [[nodiscard]] auto
    checkDeviceExtensionSupport(const std::vector<const char *> &requiredExtensions) const -> bool;

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