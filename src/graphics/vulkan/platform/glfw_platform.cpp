#include "graphics/vulkan/platform/glfw_platform.hpp"

#include "core/logger/logger.hpp"
#include "core/type.hpp"
#include "graphics/vulkan/utils/vk_utils.hpp"

#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace vostok::graphics::vulkan::platform
{

auto GlfwPlatform::getRequiredInstanceExtensions() const -> std::vector<const char *>
{
    u32 count = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&count);
    return {extensions, extensions + count};
}

auto GlfwPlatform::createSurface(VkInstance instance, void *window) const
    -> std::expected<VkSurfaceKHR, std::string>
{
    if (window == nullptr) {
        return std::unexpected("Window handle is null");
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result =
        glfwCreateWindowSurface(instance, static_cast<GLFWwindow *>(window), nullptr, &surface);
    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to create Vulkan surface: " + utils::vkResultToString(result)
        );
    }

    Logger::debug("Vulkan surface created with GLFW");
    return surface;
}

} // namespace vostok::graphics::vulkan::platform