#include "graphics/vulkan/core/vulkan_surface.hpp"

#include "graphics/vulkan/core/vulkan_instance.hpp"

namespace vostok::graphics::vulkan
{

VulkanSurface::VulkanSurface(VulkanInstance *instance, VkSurfaceKHR surface)
    : m_instance(instance),
      m_surface(surface)
{}

VulkanSurface::~VulkanSurface()
{
    if (m_surface != VK_NULL_HANDLE) {
        m_instance->destroySurface(m_surface);
    }
}

VulkanSurface::VulkanSurface(VulkanSurface &&other) noexcept
    : m_instance(other.m_instance),
      m_surface(other.m_surface)
{
    other.m_surface = VK_NULL_HANDLE;
}

auto VulkanSurface::operator=(VulkanSurface &&other) noexcept -> VulkanSurface &
{
    if (this != &other) {
        if (m_surface != VK_NULL_HANDLE) {
            m_instance->destroySurface(m_surface);
        }

        m_instance = other.m_instance;
        m_surface = other.m_surface;

        other.m_instance = nullptr;
        other.m_surface = VK_NULL_HANDLE;
    }
    return *this;
}

auto VulkanSurface::create(VulkanInstance *instance, void *windowHandle)
    -> std::expected<std::unique_ptr<VulkanSurface>, std::string>
{
    auto result = instance->createSurface(windowHandle);
    if (!result) {
        return std::unexpected(result.error());
    }

    return std::unique_ptr<VulkanSurface>(
        new VulkanSurface(instance, result.value())
    );
}

} // namespace vostok::graphics::vulkan