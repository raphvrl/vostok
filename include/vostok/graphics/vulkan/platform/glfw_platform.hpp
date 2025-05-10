#pragma once

#include "platform_interface.hpp"

namespace vostok::graphics::vulkan::platform
{

class GlfwPlatform : public PlatformInterface
{
public:
    [[nodiscard]] std::vector<const char *> getRequiredInstanceExtensions() const override;

    std::expected<VkSurfaceKHR, std::string>
    createSurface(VkInstance instance, void *window) const override;
};

} // namespace vostok::graphics::vulkan::platform