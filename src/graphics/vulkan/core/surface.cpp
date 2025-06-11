#include "graphics/vulkan/core/surface.hpp"

#include "graphics/vulkan/core/instance.hpp"

namespace vostok::graphics::vulkan
{

Surface::Surface(Instance *instance, VkSurfaceKHR surface)
    : m_instance(instance),
      m_surface(surface)
{}

Surface::~Surface()
{
    if (m_surface != VK_NULL_HANDLE) {
        m_instance->destroySurface(m_surface);
    }
}

Surface::Surface(Surface &&other) noexcept
    : m_instance(other.m_instance),
      m_surface(other.m_surface)
{
    other.m_surface = VK_NULL_HANDLE;
}

auto Surface::operator=(Surface &&other) noexcept -> Surface &
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

auto Surface::create(Instance *instance, void *windowHandle)
    -> std::expected<std::unique_ptr<Surface>, std::string>
{
    auto result = instance->createSurface(windowHandle);
    if (!result) {
        return std::unexpected(result.error());
    }

    return std::unique_ptr<Surface>(new Surface(instance, result.value()));
}

} // namespace vostok::graphics::vulkan