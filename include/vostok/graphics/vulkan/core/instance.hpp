#pragma once

#include "graphics/vulkan/platform/platform_interface.hpp"
#include "vostok/core/type.hpp"

#include <expected>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace vostok::graphics::vulkan
{

class Instance
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

    static std::expected<std::unique_ptr<Instance>, std::string>
    create(const CreateInfo &createInfo);

    ~Instance();

    Instance(const Instance &) = delete;
    Instance &operator=(const Instance &) = delete;

    Instance(Instance &&other) noexcept;
    Instance &operator=(Instance &&other) noexcept;

    [[nodiscard]] VkInstance getHandle() const
    {
        return m_instance;
    }

    [[nodiscard]] bool hasValidation() const
    {
        return m_validationEnabled;
    }

    std::expected<VkSurfaceKHR, std::string> createSurface(void *windowHandle);
    void destroySurface(VkSurfaceKHR surface);

private:
    Instance(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, bool validationEnabled);

    static bool checkValidationLayerSupport(const std::vector<const char *> &validationLayers);
    static std::vector<const char *> getRequiredExtensions(const CreateInfo &createInfo);

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    bool m_validationEnabled = false;

    std::shared_ptr<platform::PlatformInterface> m_platform;
};

} // namespace vostok::graphics::vulkan