#pragma once

#include <expected>
#include <memory>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class Instance;

class Surface
{
public:
    ~Surface();

    Surface(const Surface &) = delete;
    Surface &operator=(const Surface &) = delete;
    Surface(Surface &&other) noexcept;
    Surface &operator=(Surface &&other) noexcept;

    static std::expected<std::unique_ptr<Surface>, std::string>
    create(Instance *instance, void *windowHandle);

    [[nodiscard]] VkSurfaceKHR getHandle() const { return m_surface; }

private:
    Surface(Instance *instance, VkSurfaceKHR surface);

    Instance *m_instance = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan