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

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<Swapchain>, std::string>;

    ~Swapchain();

    Swapchain(const Swapchain &) = delete;
    auto operator=(const Swapchain &) -> Swapchain & = delete;
    Swapchain(Swapchain &&other) noexcept;
    auto operator=(Swapchain &&other) noexcept -> Swapchain &;

    [[nodiscard]] auto getHandle() const -> VkSwapchainKHR { return m_swapchain; }
    [[nodiscard]] auto getFormat() const -> VkFormat { return m_format; }
    [[nodiscard]] auto getExtent() const -> VkExtent2D { return m_extent; }
    [[nodiscard]] auto getImage(u32 index) const -> VkImage { return m_images[index]; }
    [[nodiscard]] auto getImageView(u32 index) const -> VkImageView { return m_imageViews[index]; }

    auto acquireNextImage(VkSemaphore semaphore, VkFence fence) -> std::expected<u32, std::string>;
    auto present(u32 imageIndex, VkSemaphore renderSemaphore) -> std::expected<void, std::string>;

    auto recreate(const SwapchainExtent &size)
        -> std::expected<std::unique_ptr<Swapchain>, std::string>;

private:
    Swapchain(
        Device *device,
        VkSurfaceKHR surface,
        VkSwapchainKHR swapchain,
        VkFormat format,
        VkExtent2D extent
    );

    auto createImageViews() -> bool;
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