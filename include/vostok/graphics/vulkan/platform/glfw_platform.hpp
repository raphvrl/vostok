#pragma once

#include "platform_interface.hpp"

namespace vostok::graphics::vulkan::platform
{

class GlfwPlatform : public PlatformInterface
{
public:
    [[nodiscard]] auto getRequiredInstanceExtensions() const -> std::vector<const char *> override;

    auto createSurface(VkInstance instance, void *window) const
        -> std::expected<VkSurfaceKHR, std::string> override;
};

} // namespace vostok::graphics::vulkan::platform