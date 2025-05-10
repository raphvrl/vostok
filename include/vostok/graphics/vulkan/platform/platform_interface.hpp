#pragma once

#include <expected>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan::platform
{

class PlatformInterface
{
public:
    PlatformInterface() = default;
    virtual ~PlatformInterface() = default;

    PlatformInterface(const PlatformInterface &) = delete;
    PlatformInterface &operator=(const PlatformInterface &) = delete;
    PlatformInterface(PlatformInterface &&) = delete;
    PlatformInterface &operator=(PlatformInterface &&) = delete;

    [[nodiscard]] virtual std::vector<const char *> getRequiredInstanceExtensions() const = 0;

    virtual std::expected<VkSurfaceKHR, std::string>
    createSurface(VkInstance instance, void *window) const = 0;
};

} // namespace vostok::graphics::vulkan::platform