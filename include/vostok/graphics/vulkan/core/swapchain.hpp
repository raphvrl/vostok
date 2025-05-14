#pragma once

#include "graphics/vulkan/core/device.hpp"

#include <expected>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

struct SwapchainExtent
{
    u32 width = 0;
    u32 height = 0;
};

class Swapchain
{
public:
    struct CreateInfo
    {
        Device *device = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        u32 width = 0;
        u32 height = 0;
        u32 imageCount = 2;
        bool vsync = false;
    };

    static std::expected<std::unique_ptr<Swapchain>, std::string>
    create(const CreateInfo &createInfo);

    ~Swapchain();

    Swapchain(const Swapchain &) = delete;
    Swapchain &operator=(const Swapchain &) = delete;
    Swapchain(Swapchain &&other) noexcept;
    Swapchain &operator=(Swapchain &&other) noexcept;

    [[nodiscard]] VkSwapchainKHR getHandle() const { return m_swapchain; }
    [[nodiscard]] VkFormat getFormat() const { return m_format; }
    [[nodiscard]] VkExtent2D getExtent() const { return m_extent; }
    [[nodiscard]] VkImage getImage(u32 index) const { return m_images[index]; }
    [[nodiscard]] VkImageView getImageView(u32 index) const { return m_imageViews[index]; }

    std::expected<u32, std::string> acquireNextImage(VkSemaphore semaphore, VkFence fence);
    std::expected<void, std::string> present(u32 imageIndex, VkSemaphore renderSemaphore);

    std::expected<std::unique_ptr<Swapchain>, std::string> recreate(const SwapchainExtent &size);

private:
    Swapchain(
        Device *device,
        VkSurfaceKHR surface,
        VkSwapchainKHR swapchain,
        VkFormat format,
        VkExtent2D extent
    );

    bool createImageViews();
    void cleanup();

    Device *m_device = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent = {};

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
};

}; // namespace vostok::graphics::vulkan