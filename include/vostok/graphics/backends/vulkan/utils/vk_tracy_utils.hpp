#pragma once

// clang-format off
#ifdef VOSTOK_ENABLE_TRACY
#include <vulkan/vulkan.h>
#include <volk.h>
#include <tracy/TracyVulkan.hpp>
#endif
// clang-format on

namespace vostok::graphics::vulkan::utils
{

inline constexpr bool TRACY_ENABLED =
#ifdef VOSTOK_ENABLE_TRACY
    true;
#else
    false;
#endif

[[nodiscard]] constexpr auto isTracyEnabled() noexcept -> bool
{
    return TRACY_ENABLED;
}

template <typename Func>
constexpr void ifTracyEnabled(Func &&func) noexcept
{
    if constexpr (TRACY_ENABLED) {
        std::forward<Func>(func)();
    }
}

template <typename Func>
constexpr void ifTracyEnabled(tracy::VkCtx *context, Func &&func) noexcept
{
    if constexpr (TRACY_ENABLED) {
        if (context != nullptr) {
            std::forward<Func>(func)();
        }
    }
}

template <typename Func>
constexpr void ifTracyEnabled(
    tracy::VkCtx *context,
    VkCommandBuffer commandBuffer,
    Func &&func
) noexcept
{
    if constexpr (TRACY_ENABLED) {
        if (context != nullptr && commandBuffer != VK_NULL_HANDLE) {
            std::forward<Func>(func)();
        }
    }
}

} // namespace vostok::graphics::vulkan::utils
