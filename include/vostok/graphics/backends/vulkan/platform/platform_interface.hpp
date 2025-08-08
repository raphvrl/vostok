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
    auto operator=(const PlatformInterface &) -> PlatformInterface & = delete;
    PlatformInterface(PlatformInterface &&) = delete;
    auto operator=(PlatformInterface &&) -> PlatformInterface & = delete;

    [[nodiscard]] virtual auto getRequiredInstanceExtensions() const
        -> std::vector<const char *> = 0;

    virtual auto createSurface(VkInstance instance, void *window) const
        -> std::expected<VkSurfaceKHR, std::string> = 0;
};

} // namespace vostok::graphics::vulkan::platform