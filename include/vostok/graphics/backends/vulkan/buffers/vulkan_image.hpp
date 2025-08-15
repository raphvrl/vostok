#pragma once

#include "vostok/graphics/backends/vulkan/core/vulkan_allocator.hpp"
#include "vostok/graphics/buffers/buffer.hpp"
#include "vostok/graphics/buffers/image.hpp"

#include <memory>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanInstance;
class VulkanDevice;
class VulkanAllocator;
class VulkanFrameSync;

class VulkanImage : public graphics::Image
{
public:
    struct ImageDimensions
    {
        u32 width;
        u32 height;
        u32 depth;

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        ImageDimensions(u32 w, u32 h, u32 d)
            : width(w),
              height(h),
              depth(d)
        {}
    };

    struct LayoutTransitionRequest
    {
        VkImageLayout sourceLayout;
        VkImageLayout targetLayout;
        VkImageAspectFlags aspectMask;

        LayoutTransitionRequest(
            // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
            VkImageLayout source,
            VkImageLayout target,
            VkImageAspectFlags aspect
        )
            : sourceLayout(source),
              targetLayout(target),
              aspectMask(aspect)
        {}
    };

    static auto create(
        VulkanDevice *device,
        VulkanAllocator *allocator,
        VulkanFrameSync *frameSync,
        const graphics::ImageCreateInfo &info
    ) -> std::expected<std::unique_ptr<VulkanImage>, std::string>;

    static auto createAndTransfer(
        VulkanDevice *device,
        VulkanAllocator *allocator,
        VulkanFrameSync *frameSync,
        const graphics::ImageCreateInfo &createInfo,
        std::unique_ptr<graphics::Buffer> stagingBuffer
    ) -> std::expected<std::unique_ptr<VulkanImage>, std::string>;

    ~VulkanImage() override;

    VulkanImage(const VulkanImage &) = delete;
    auto operator=(const VulkanImage &) -> VulkanImage & = delete;
    VulkanImage(VulkanImage &&other) noexcept;
    auto operator=(VulkanImage &&other) noexcept -> VulkanImage &;

    auto transitionLayout(
        graphics::ImageLayout oldLayout,
        graphics::ImageLayout newLayout,
        graphics::ImageAspectFlags aspectMask =
            graphics::ImageAspectFlags::COLOR
    ) -> std::expected<void, std::string> override;

    auto clear(
        const graphics::ClearValue &clearValue,
        graphics::ImageAspectFlags aspectMask =
            graphics::ImageAspectFlags::COLOR
    ) -> std::expected<void, std::string> override;

    [[nodiscard]] auto getWidth() const -> u32 override { return m_width; }
    [[nodiscard]] auto getHeight() const -> u32 override { return m_height; }
    [[nodiscard]] auto getDepth() const -> u32 override { return m_depth; }
    [[nodiscard]] auto getFormat() const -> graphics::ImageFormat override
    {
        return m_format;
    }
    [[nodiscard]] auto getUsage() const -> graphics::ImageUsage override
    {
        return m_usage;
    }
    [[nodiscard]] auto getMipLevels() const -> u32 override
    {
        return m_mipLevels;
    }
    [[nodiscard]] auto getArrayLayers() const -> u32 override
    {
        return m_arrayLayers;
    }

    [[nodiscard]] auto getImage() const -> VkImage { return m_image; }
    [[nodiscard]] auto getAllocation() const -> VmaAllocation
    {
        return m_allocation;
    }
    [[nodiscard]] auto getImageView() const -> VkImageView
    {
        return m_imageView;
    }
    [[nodiscard]] auto getCurrentLayout() const -> VkImageLayout
    {
        return m_currentLayout;
    }

    [[nodiscard]] auto isDepthStencil() const -> bool override
    {
        return m_format == graphics::ImageFormat::D32_SFLOAT ||
               m_format == graphics::ImageFormat::D24_UNORM_S8_UINT;
    }

    [[nodiscard]] auto isColor() const -> bool override
    {
        return !isDepthStencil();
    }

    [[nodiscard]] auto getAspectMask() const
        -> graphics::ImageAspectFlags override
    {
        if (m_format == graphics::ImageFormat::D32_SFLOAT) {
            return graphics::ImageAspectFlags::DEPTH;
        }
        if (m_format == graphics::ImageFormat::D24_UNORM_S8_UINT) {
            return graphics::ImageAspectFlags::DEPTH_STENCIL;
        }
        return graphics::ImageAspectFlags::COLOR;
    }

    auto createImageView(VkImageAspectFlags aspectMask)
        -> std::expected<void, std::string>;
    auto destroyImageView() -> void;

private:
    VulkanImage(
        VulkanDevice *device,
        VulkanAllocator *allocator,
        VulkanFrameSync *frameSync,
        VkImage image,
        VmaAllocation allocation,
        const graphics::ImageCreateInfo &info
    );

    static constexpr VkImageLayout DEFAULT_CLEAR_LAYOUT =
        VK_IMAGE_LAYOUT_GENERAL;
    static constexpr VkImageLayout COLOR_CLEAR_LAYOUT =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    static constexpr VkImageLayout DEPTH_CLEAR_LAYOUT =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    auto destroyImage() -> void;
    auto createImageViewInternal(VkImageAspectFlags aspectMask) -> VkImageView;

    auto transitionLayoutInternal(
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkImageAspectFlags aspectMask,
        bool useTransferCommandBuffer = false
    ) -> std::expected<void, std::string>;

    VulkanDevice *m_device = nullptr;
    VulkanAllocator *m_allocator = nullptr;
    VulkanFrameSync *m_frameSync = nullptr;
    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    u32 m_width = 0;
    u32 m_height = 0;
    u32 m_depth = 1;
    graphics::ImageFormat m_format = graphics::ImageFormat::UNDEFINED;
    graphics::ImageUsage m_usage = graphics::ImageUsage::COLOR_ATTACHMENT;
    u32 m_mipLevels = 1;
    u32 m_arrayLayers = 1;
    VkSampleCountFlagBits m_samples = VK_SAMPLE_COUNT_1_BIT;

    static auto determineImageType(const ImageDimensions &dimensions)
        -> VkImageType;

    struct LayoutTransitionInfo
    {
        VkPipelineStageFlags2 srcStageMask;
        VkAccessFlags2 srcAccessMask;
        VkPipelineStageFlags2 dstStageMask;
        VkAccessFlags2 dstAccessMask;
    };

    static auto
    getLayoutTransitionStages(const LayoutTransitionRequest &request)
        -> LayoutTransitionInfo;

    auto transferFromBuffer(const graphics::Buffer &buffer)
        -> std::expected<void, std::string>;

    friend class VulkanGPU;
};

} // namespace vostok::graphics::vulkan