#pragma once

#include <expected>
#include <memory>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanInstance;

class VulkanSurface
{
public:
    ~VulkanSurface();

    VulkanSurface(const VulkanSurface &) = delete;
    auto operator=(const VulkanSurface &) -> VulkanSurface & = delete;
    VulkanSurface(VulkanSurface &&other) noexcept;
    auto operator=(VulkanSurface &&other) noexcept -> VulkanSurface &;

    static auto create(VulkanInstance *instance, void *windowHandle)
        -> std::expected<std::unique_ptr<VulkanSurface>, std::string>;

    [[nodiscard]] auto getHandle() const -> VkSurfaceKHR { return m_surface; }

private:
    VulkanSurface(VulkanInstance *instance, VkSurfaceKHR surface);

    VulkanInstance *m_instance = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan