#pragma once

#include "graphics/backends/vulkan/platform/platform_interface.hpp"
#include "vostok/core/type.hpp"

#include <expected>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace vostok::graphics::vulkan
{

class VulkanInstance
{
public:
    struct CreateInfo
    {
        std::string appName = "Vostok app";
        u32 appVersion = VK_MAKE_VERSION(1, 0, 0);
        std::string engineName = "Vostok engine";
        u32 engineVersion = VK_MAKE_VERSION(1, 0, 0);
        u32 apiVersion = VK_API_VERSION_1_4;
        bool enableValidationLayers = true;
        std::vector<const char *> validationLayers;
        std::shared_ptr<platform::PlatformInterface> platform;
    };

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<VulkanInstance>, std::string>;

    ~VulkanInstance();

    VulkanInstance(const VulkanInstance &) = delete;
    auto operator=(const VulkanInstance &) -> VulkanInstance & = delete;

    VulkanInstance(VulkanInstance &&other) noexcept;
    auto operator=(VulkanInstance &&other) noexcept -> VulkanInstance &;

    [[nodiscard]] auto getHandle() const -> VkInstance { return m_instance; }

    [[nodiscard]] auto hasValidation() const -> bool
    {
        return m_validationEnabled;
    }

    auto createSurface(void *windowHandle)
        -> std::expected<VkSurfaceKHR, std::string>;
    void destroySurface(VkSurfaceKHR surface);

private:
    VulkanInstance(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        bool validationEnabled,
        std::shared_ptr<platform::PlatformInterface> platform
    );

    static auto checkValidationLayerSupport(
        const std::vector<const char *> &validationLayers
    ) -> bool;
    static auto getRequiredExtensions(const CreateInfo &createInfo)
        -> std::vector<const char *>;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    bool m_validationEnabled = false;

    std::shared_ptr<platform::PlatformInterface> m_platform;
};

} // namespace vostok::graphics::vulkan