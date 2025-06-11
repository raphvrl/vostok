#include "graphics/vulkan/core/instance.hpp"

#include "core/logger/logger.hpp"
#include "graphics/vulkan/platform/platform_interface.hpp"
#include "graphics/vulkan/utils/vk_logger.hpp"
#include "graphics/vulkan/utils/vk_utils.hpp"

#include <cstring>
#include <memory>
#include <utility>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan_core.h>

namespace vostok::graphics::vulkan
{

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    [[maybe_unused]] void *pUserData
)
{
    auto &logger = utils::getVulkanLogger();

    if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) {
        logger.error("{}", pCallbackData->pMessage);
    } else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0) {
        logger.warning("{}", pCallbackData->pMessage);
    } else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0) {
        logger.info("{}", pCallbackData->pMessage);
    } else {
        logger.debug("{}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

Instance::Instance(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    bool validationEnabled,
    std::shared_ptr<platform::PlatformInterface> platform
)
    : m_instance(instance),
      m_debugMessenger(debugMessenger),
      m_validationEnabled(validationEnabled),
      m_platform(std::move(platform))
{}

Instance::~Instance()
{
    if (m_debugMessenger != VK_NULL_HANDLE) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT")
        );

        if (func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
    }

    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
    }

    Logger::debug("Vulkan instance destroyed");
}

Instance::Instance(Instance &&other) noexcept
    : m_instance(other.m_instance),
      m_debugMessenger(other.m_debugMessenger),
      m_validationEnabled(other.m_validationEnabled)
{
    other.m_instance = VK_NULL_HANDLE;
    other.m_debugMessenger = VK_NULL_HANDLE;
}

auto Instance::operator=(Instance &&other) noexcept -> Instance &
{
    if (this != &other) {
        m_instance = other.m_instance;
        m_debugMessenger = other.m_debugMessenger;
        m_validationEnabled = other.m_validationEnabled;

        other.m_instance = VK_NULL_HANDLE;
        other.m_debugMessenger = VK_NULL_HANDLE;
    }
    return *this;
}

auto Instance::create(const CreateInfo &createInfo)
    -> std::expected<std::unique_ptr<Instance>, std::string>
{
    VkResult volkResult = volkInitialize();
    if (volkResult != VK_SUCCESS) {
        return std::unexpected("Failed to initialize volk: " + utils::vkResultToString(volkResult));
    }

    Logger::info(
        "Creating Vulkan instance (API version: {}.{}.{})",
        VK_VERSION_MAJOR(createInfo.apiVersion),
        VK_VERSION_MINOR(createInfo.apiVersion),
        VK_VERSION_PATCH(createInfo.apiVersion)
    );

    bool validationEnabled = createInfo.enableValidationLayers;
    std::vector<const char *> validationLayers;

    if (validationEnabled) {
        validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        if (!checkValidationLayerSupport(validationLayers)) {
            Logger::warning("Validation layers requested, but not available!");
            validationEnabled = false;
            validationLayers.clear();
        }
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = createInfo.appName.c_str();
    appInfo.applicationVersion = createInfo.appVersion;
    appInfo.pEngineName = createInfo.engineName.c_str();
    appInfo.engineVersion = createInfo.engineVersion;
    appInfo.apiVersion = createInfo.apiVersion;

    std::vector<const char *> extensions = getRequiredExtensions(createInfo);

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<u32>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
    instanceCreateInfo.enabledLayerCount = static_cast<u32>(validationLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    if (validationEnabled) {
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        instanceCreateInfo.pNext = &debugCreateInfo;
    }

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to create Vulkan instance " + utils::vkResultToString(result)
        );
    }

    volkLoadInstance(instance);

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    if (validationEnabled) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
        );

        if (func != nullptr) {
            result = func(instance, &debugCreateInfo, nullptr, &debugMessenger);
            if (result != VK_SUCCESS) {
                Logger::warning(
                    "Failed to set up debug messenger: {}",
                    utils::vkResultToString(result)
                );
            }
        } else {
            Logger::warning("Could not find vkCreateDebugUtilsMessengerEXT function");
        }
    }

    auto instancePtr = std::unique_ptr<Instance>(
        new Instance(instance, debugMessenger, validationEnabled, createInfo.platform)
    );

    Logger::info("Vulkan instance created");
    if (validationEnabled) {
        Logger::debug("Vulkan validation layers enabled");
    }

    return instancePtr;
}

auto Instance::createSurface(void *windowHandle) -> std::expected<VkSurfaceKHR, std::string>
{
    if (m_platform == nullptr) {
        return std::unexpected("No platform interface available");
    }

    return m_platform->createSurface(m_instance, windowHandle);
}

void Instance::destroySurface(VkSurfaceKHR surface)
{
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, surface, nullptr);
        Logger::debug("Vulkan surface destroyed");
    }
}

auto Instance::checkValidationLayerSupport(const std::vector<const char *> &validationLayers)
    -> bool
{
    u32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers) {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, static_cast<const char *>(layerProperties.layerName)) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            Logger::warning("Validation layer {} not found", layerName);
            return false;
        }
    }

    return true;
}

auto Instance::getRequiredExtensions(const CreateInfo &createInfo) -> std::vector<const char *>
{
    if (createInfo.platform == nullptr) {
        Logger::warning("No platform interface available");
        return {};
    }

    std::vector<const char *> extensions = createInfo.platform->getRequiredInstanceExtensions();

    if (createInfo.enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    for (const char *extension : extensions) {
        Logger::debug("Vulkan extension: {}", extension);
    }

    return extensions;
}

} // namespace vostok::graphics::vulkan