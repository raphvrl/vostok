#pragma once

#include "graphics/backends/vulkan/core/vulkan_device.hpp"

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

class VulkanSwapchain
{
public:
    struct CreateInfo
    {
        VulkanDevice *device = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        u32 width = 0;
        u32 height = 0;
        u32 imageCount = 2;
        bool vsync = false;
    };

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<VulkanSwapchain>, std::string>;

    ~VulkanSwapchain();

    VulkanSwapchain(const VulkanSwapchain &) = delete;
    auto operator=(const VulkanSwapchain &) -> VulkanSwapchain & = delete;
    VulkanSwapchain(VulkanSwapchain &&other) noexcept;
    auto operator=(VulkanSwapchain &&other) noexcept -> VulkanSwapchain &;

    [[nodiscard]] auto getHandle() const -> VkSwapchainKHR
    {
        return m_swapchain;
    }
    [[nodiscard]] auto getFormat() const -> VkFormat { return m_format; }
    [[nodiscard]] auto getExtent() const -> VkExtent2D { return m_extent; }
    [[nodiscard]] auto getImage(u32 index) const -> VkImage
    {
        return m_images[index];
    }
    [[nodiscard]] auto getImageView(u32 index) const -> VkImageView
    {
        return m_imageViews[index];
    }

    auto acquireNextImage(VkSemaphore semaphore, VkFence fence)
        -> std::expected<u32, std::string>;
    auto present(u32 imageIndex, VkSemaphore renderSemaphore)
        -> std::expected<void, std::string>;

    auto recreate(const SwapchainExtent &size)
        -> std::expected<std::unique_ptr<VulkanSwapchain>, std::string>;

private:
    VulkanSwapchain(
        VulkanDevice *device,
        VkSurfaceKHR surface,
        VkSwapchainKHR swapchain,
        VkFormat format,
        VkExtent2D extent
    );

    auto createImageViews() -> bool;
    void cleanup();

    VulkanDevice *m_device = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_extent = {};

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
};

}; // namespace vostok::graphics::vulkan