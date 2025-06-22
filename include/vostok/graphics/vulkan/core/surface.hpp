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
    auto operator=(const Surface &) -> Surface & = delete;
    Surface(Surface &&other) noexcept;
    auto operator=(Surface &&other) noexcept -> Surface &;

    static auto create(Instance *instance, void *windowHandle)
        -> std::expected<std::unique_ptr<Surface>, std::string>;

    [[nodiscard]] auto getHandle() const -> VkSurfaceKHR { return m_surface; }

private:
    Surface(Instance *instance, VkSurfaceKHR surface);

    Instance *m_instance = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan