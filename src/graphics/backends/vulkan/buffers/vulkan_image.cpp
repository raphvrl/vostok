#include "vostok/graphics/backends/vulkan/buffers/vulkan_image.hpp"

#include "vostok/core/logger/logger.hpp"
#include "vostok/graphics/backends/vulkan/buffers/vulkan_buffer.hpp"
#include "vostok/graphics/backends/vulkan/core/vulkan_device.hpp"
#include "vostok/graphics/backends/vulkan/core/vulkan_frame_sync.hpp"
#include "vostok/graphics/backends/vulkan/utils/vk_utils.hpp"

#include <format>
#include <vk_mem_alloc.h>
#include <volk.h>

namespace vostok::graphics::vulkan
{

auto VulkanImage::create(
    VulkanDevice *device,
    VulkanAllocator *allocator,
    VulkanFrameSync *frameSync,
    const graphics::ImageCreateInfo &info
) -> std::expected<std::unique_ptr<VulkanImage>, std::string>
{
    if (info.width == 0 || info.height == 0) {
        return std::unexpected(
            "Invalid image dimensions: width and height must be > 0"
        );
    }

    if (info.depth == 0) {
        return std::unexpected("Invalid image depth: must be > 0");
    }

    if (info.mipLevels == 0) {
        return std::unexpected("Invalid mip levels: must be > 0");
    }

    if (info.arrayLayers == 0) {
        return std::unexpected("Invalid array layers: must be > 0");
    }

    VkFormat vkFormat = utils::toVulkanFormat(info.format);
    if (vkFormat == VK_FORMAT_UNDEFINED) {
        return std::unexpected(
            "Unsupported image format: " +
            std::to_string(static_cast<int>(info.format))
        );
    }

    VkSampleCountFlagBits vkSamples = utils::toVulkanSampleCount(info.samples);
    VkImageUsageFlags vkUsage = utils::toVulkanImageUsage(info.usage);

    VkImageAspectFlags aspectMaskPre = VK_IMAGE_ASPECT_COLOR_BIT;
    if (info.format == graphics::ImageFormat::D32_SFLOAT) {
        aspectMaskPre = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else if (info.format == graphics::ImageFormat::D24_UNORM_S8_UINT) {
        aspectMaskPre = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if ((aspectMaskPre & VK_IMAGE_ASPECT_DEPTH_BIT) != 0U) {
        vkUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        vkUsage &= ~static_cast<VkImageUsageFlags>(
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        );
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = determineImageType(
        ImageDimensions{ info.width, info.height, info.depth }
    );
    imageInfo.extent.width = info.width;
    imageInfo.extent.height = info.height;
    imageInfo.extent.depth = info.depth;
    imageInfo.mipLevels = info.mipLevels;
    imageInfo.arrayLayers = info.arrayLayers;
    imageInfo.format = vkFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = vkUsage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = vkSamples;

    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

    auto result = allocator->createImage(imageInfo, memoryUsage);
    if (!result) {
        return std::unexpected("Failed to create image: " + result.error());
    }

    auto [image, allocation] = result.value();

    auto vulkanImage = std::unique_ptr<VulkanImage>(
        new VulkanImage(device, allocator, frameSync, image, allocation, info)
    );

    VkImageAspectFlags aspectMask = aspectMaskPre;

    auto viewResult = vulkanImage->createImageView(aspectMask);
    if (!viewResult) {
        return std::unexpected(
            "Failed to create image view: " + viewResult.error()
        );
    }

    Logger::debug(
        "Created Vulkan image: {}x{}x{}, format: {}, usage: {}",
        info.width,
        info.height,
        info.depth,
        static_cast<int>(info.format),
        static_cast<int>(info.usage)
    );

    return vulkanImage;
}

auto VulkanImage::createAndTransfer(
    VulkanDevice *device,
    VulkanAllocator *allocator,
    VulkanFrameSync *frameSync,
    const graphics::ImageCreateInfo &createInfo,
    std::unique_ptr<graphics::Buffer> stagingBuffer
) -> std::expected<std::unique_ptr<VulkanImage>, std::string>
{
    auto imageResult = create(device, allocator, frameSync, createInfo);
    if (!imageResult) {
        return std::unexpected{
            std::format("Failed to create image: {}", imageResult.error())
        };
    }

    auto vulkanImage = std::move(imageResult.value());

    auto transferResult = vulkanImage->transferFromBuffer(*stagingBuffer);
    if (!transferResult) {
        return std::unexpected{ "Failed to transfer data: " +
                                transferResult.error() };
    }

    Logger::debug(
        "Created and transferred Vulkan image: {}x{}x{}, format: {}",
        createInfo.width,
        createInfo.height,
        createInfo.depth,
        static_cast<int>(createInfo.format)
    );

    return vulkanImage;
}

VulkanImage::VulkanImage(
    VulkanDevice *device,
    VulkanAllocator *allocator,
    VulkanFrameSync *frameSync,
    VkImage image,
    VmaAllocation allocation,
    const graphics::ImageCreateInfo &info
)
    : m_device(device),
      m_allocator(allocator),
      m_frameSync(frameSync),
      m_image(image),
      m_allocation(allocation),
      m_width(info.width),
      m_height(info.height),
      m_depth(info.depth),
      m_format(info.format),
      m_usage(info.usage),
      m_mipLevels(info.mipLevels),
      m_arrayLayers(info.arrayLayers),
      m_samples(utils::toVulkanSampleCount(info.samples))
{}

VulkanImage::~VulkanImage()
{
    destroyImage();
}

VulkanImage::VulkanImage(VulkanImage &&other) noexcept
    : m_device(other.m_device),
      m_allocator(other.m_allocator),
      m_frameSync(other.m_frameSync),
      m_image(other.m_image),
      m_allocation(other.m_allocation),
      m_imageView(other.m_imageView),
      m_currentLayout(other.m_currentLayout),
      m_width(other.m_width),
      m_height(other.m_height),
      m_depth(other.m_depth),
      m_format(other.m_format),
      m_usage(other.m_usage),
      m_mipLevels(other.m_mipLevels),
      m_arrayLayers(other.m_arrayLayers),
      m_samples(other.m_samples)
{
    other.m_image = VK_NULL_HANDLE;
    other.m_allocation = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

auto VulkanImage::operator=(VulkanImage &&other) noexcept -> VulkanImage &
{
    if (this != &other) {
        destroyImage();

        m_device = other.m_device;
        m_allocator = other.m_allocator;
        m_image = other.m_image;
        m_allocation = other.m_allocation;
        m_imageView = other.m_imageView;
        m_currentLayout = other.m_currentLayout;
        m_width = other.m_width;
        m_height = other.m_height;
        m_depth = other.m_depth;
        m_format = other.m_format;
        m_usage = other.m_usage;
        m_mipLevels = other.m_mipLevels;
        m_arrayLayers = other.m_arrayLayers;
        m_samples = other.m_samples;

        other.m_image = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
    return *this;
}

auto VulkanImage::transitionLayout(
    graphics::ImageLayout oldLayout,
    graphics::ImageLayout newLayout,
    graphics::ImageAspectFlags aspectMask
) -> std::expected<void, std::string>
{
    if (m_frameSync == nullptr) {
        return std::unexpected("FrameSync is required for layout transitions");
    }

    VkImageLayout vkOldLayout = utils::toVulkanImageLayout(oldLayout);
    VkImageLayout vkNewLayout = utils::toVulkanImageLayout(newLayout);
    VkImageAspectFlags vkAspectMask = utils::toVulkanAspectFlags(aspectMask);

    if (vkOldLayout == vkNewLayout) {
        Logger::debug("Image layout transition skipped: same layout");
        return {};
    }

    auto beginResult = m_frameSync->beginCommandBuffer();
    if (!beginResult) {
        return std::unexpected(
            "Failed to begin command buffer: " + beginResult.error()
        );
    }

    auto transitionResult =
        transitionLayoutInternal(vkOldLayout, vkNewLayout, vkAspectMask, false);
    if (!transitionResult) {
        return std::unexpected(
            "Failed to transition image layout: " + transitionResult.error()
        );
    }

    auto endResult = m_frameSync->endCommandBuffer();
    if (!endResult) {
        return std::unexpected(
            "Failed to end command buffer: " + endResult.error()
        );
    }

    auto submitResult = m_frameSync->submitCommandBuffer();
    if (!submitResult) {
        return std::unexpected(
            "Failed to submit command buffer: " + submitResult.error()
        );
    }

    auto waitResult = m_frameSync->waitForComplete();
    if (!waitResult) {
        return std::unexpected(
            "Failed to wait for completion: " + waitResult.error()
        );
    }

    m_currentLayout = vkNewLayout;

    Logger::debug(
        "Image layout transition: {} -> {}",
        static_cast<int>(oldLayout),
        static_cast<int>(newLayout)
    );

    return {};
}

auto VulkanImage::transitionLayoutInternal(
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkImageAspectFlags aspectMask,
    bool useTransferCommandBuffer
) -> std::expected<void, std::string>
{
    if (oldLayout == newLayout) {
        return {};
    }

    auto [srcStageMask, srcAccessMask, dstStageMask, dstAccessMask] =
        getLayoutTransitionStages(
            LayoutTransitionRequest{ oldLayout, newLayout, aspectMask }
        );

    VkImageMemoryBarrier2 imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.srcStageMask = srcStageMask;
    imageBarrier.srcAccessMask = srcAccessMask;
    imageBarrier.dstStageMask = dstStageMask;
    imageBarrier.dstAccessMask = dstAccessMask;
    imageBarrier.oldLayout = oldLayout;
    imageBarrier.newLayout = newLayout;
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.image = m_image;
    imageBarrier.subresourceRange.aspectMask = aspectMask;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = m_mipLevels;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = m_arrayLayers;

    VkDependencyInfo dependencyInfo = {};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;

    VkCommandBuffer commandBuffer = useTransferCommandBuffer
                                      ? m_frameSync->getTransferCommandBuffer()
                                      : m_frameSync->getCommandBuffer();

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    return {};
}

auto VulkanImage::clear(
    const graphics::ClearValue &clearValue,
    graphics::ImageAspectFlags aspectMask
) -> std::expected<void, std::string>
{
    if (m_frameSync == nullptr) {
        return std::unexpected("FrameSync is required for image operations");
    }

    VkImageAspectFlags vkAspectMask = utils::toVulkanAspectFlags(aspectMask);

    VkClearValue vkClearValue = {};

    if (clearValue.isColor()) {
        const auto &color = clearValue.getColor();
        vkClearValue.color.float32[0] = color.r;
        vkClearValue.color.float32[1] = color.g;
        vkClearValue.color.float32[2] = color.b;
        vkClearValue.color.float32[3] = color.a;
    } else if (clearValue.isDepthStencil()) {
        const auto &depthStencil = clearValue.getDepthStencil();
        vkClearValue.depthStencil.depth = depthStencil.depth;
        vkClearValue.depthStencil.stencil = depthStencil.stencil;
    }

    VkImageLayout clearLayout = DEFAULT_CLEAR_LAYOUT;
    if (isColor()) {
        clearLayout = COLOR_CLEAR_LAYOUT;
    } else if (isDepthStencil()) {
        clearLayout = DEPTH_CLEAR_LAYOUT;
    }

    auto beginResult = m_frameSync->beginCommandBuffer();
    if (!beginResult) {
        return std::unexpected(
            "Failed to begin command buffer: " + beginResult.error()
        );
    }

    VkImageLayout originalLayout = m_currentLayout;
    if (m_currentLayout != clearLayout) {
        auto transitionResult = transitionLayoutInternal(
            m_currentLayout,
            clearLayout,
            vkAspectMask,
            false
        );
        if (!transitionResult) {
            return std::unexpected(
                "Failed to transition to clear layout: " +
                transitionResult.error()
            );
        }
        m_currentLayout = clearLayout;
    }

    if (isColor()) {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = m_mipLevels;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = m_arrayLayers;

        vkCmdClearColorImage(
            m_frameSync->getCommandBuffer(),
            m_image,
            clearLayout,
            &vkClearValue.color,
            1,
            &subresourceRange
        );
    } else if (isDepthStencil()) {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = vkAspectMask;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = m_mipLevels;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = m_arrayLayers;

        vkCmdClearDepthStencilImage(
            m_frameSync->getCommandBuffer(),
            m_image,
            clearLayout,
            &vkClearValue.depthStencil,
            1,
            &subresourceRange
        );
    }

    if (m_currentLayout != originalLayout) {
        auto transitionResult = transitionLayoutInternal(
            clearLayout,
            originalLayout,
            vkAspectMask,
            false
        );
        if (!transitionResult) {
            return std::unexpected(
                "Failed to transition back to original layout: " +
                transitionResult.error()
            );
        }
        m_currentLayout = originalLayout;
    }

    auto endResult = m_frameSync->endCommandBuffer();
    if (!endResult) {
        return std::unexpected(
            "Failed to end command buffer: " + endResult.error()
        );
    }

    auto submitResult = m_frameSync->submitCommandBuffer();
    if (!submitResult) {
        return std::unexpected(
            "Failed to submit command buffer: " + submitResult.error()
        );
    }

    auto waitResult = m_frameSync->waitForComplete();
    if (!waitResult) {
        return std::unexpected(
            "Failed to wait for completion: " + waitResult.error()
        );
    }

    Logger::debug(
        "Image clear completed: {}x{}x{}, aspect mask: {}",
        m_width,
        m_height,
        m_depth,
        static_cast<int>(aspectMask)
    );

    return {};
}

auto VulkanImage::createImageView(VkImageAspectFlags aspectMask)
    -> std::expected<void, std::string>
{
    if (m_image == VK_NULL_HANDLE) {
        return std::unexpected("Cannot create image view: image is null");
    }

    if (m_imageView != VK_NULL_HANDLE) {
        destroyImageView();
    }

    m_imageView = createImageViewInternal(aspectMask);
    if (m_imageView == VK_NULL_HANDLE) {
        return std::unexpected("Failed to create image view");
    }

    return {};
}

auto VulkanImage::destroyImageView() -> void
{
    if (m_imageView != VK_NULL_HANDLE && m_device != nullptr) {
        vkDestroyImageView(m_device->getHandle(), m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }
}

auto VulkanImage::createImageViewInternal(VkImageAspectFlags aspectMask)
    -> VkImageView
{
    if (m_image == VK_NULL_HANDLE || m_device == nullptr) {
        Logger::error("Cannot create image view: image or device is null");
        return VK_NULL_HANDLE;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType =
        m_arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = utils::toVulkanFormat(m_format);
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_arrayLayers;

    VkImageView imageView = VK_NULL_HANDLE;

    VkResult result = vkCreateImageView(
        m_device->getHandle(),
        &viewInfo,
        nullptr,
        &imageView
    );

    if (result != VK_SUCCESS) {
        Logger::error(
            "Failed to create image view: {}",
            utils::vkResultToString(result)
        );
        return VK_NULL_HANDLE;
    }

    return imageView;
}

auto VulkanImage::destroyImage() -> void
{
    destroyImageView();

    if (m_image != VK_NULL_HANDLE && m_allocator != nullptr) {
        m_allocator->destroyImage(m_image, m_allocation);
        m_image = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }
}

auto VulkanImage::determineImageType(const ImageDimensions &dimensions)
    -> VkImageType
{
    if (dimensions.width == 0 || dimensions.height == 0) {
        Logger::warning(
            "Invalid image dimensions: {}x{}x{}",
            dimensions.width,
            dimensions.height,
            dimensions.depth
        );
    }

    if (dimensions.depth > 1U) {
        return VK_IMAGE_TYPE_3D;
    }
    if (dimensions.height > 1) {
        return VK_IMAGE_TYPE_2D;
    }
    return VK_IMAGE_TYPE_1D;
}

auto VulkanImage::getLayoutTransitionStages(
    const LayoutTransitionRequest &request
) -> LayoutTransitionInfo
{
    LayoutTransitionInfo info = {};

    switch (request.sourceLayout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            info.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            info.srcAccessMask = 0;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            info.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            info.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            info.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            info.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            info.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            info.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            info.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
            info.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            break;
        default:
            info.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            info.srcAccessMask =
                VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            break;
    }

    switch (request.targetLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            info.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            info.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            info.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            info.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            info.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            info.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            info.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            info.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            info.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
            info.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            info.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            info.dstAccessMask = 0;
            break;
        default:
            info.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            info.dstAccessMask =
                VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            break;
    }

    return info;
}

auto VulkanImage::transferFromBuffer(const graphics::Buffer &stagingBuffer)
    -> std::expected<void, std::string>
{
    if (m_frameSync == nullptr) {
        return std::unexpected{ "FrameSync is required for buffer transfer" };
    }

    auto beginResult = m_frameSync->beginTransferCommandBuffer();
    if (!beginResult) {
        return std::unexpected{ "Failed to begin transfer command buffer: " +
                                beginResult.error() };
    }

    auto aspectMask = utils::toVulkanAspectFlags(getAspectMask());

    auto transitionResult = transitionLayoutInternal(
        m_currentLayout,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        aspectMask,
        true
    );
    if (!transitionResult) {
        return std::unexpected{ "Failed to transition to transfer layout: " +
                                transitionResult.error() };
    }

    VkBufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = aspectMask;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = m_arrayLayers;
    copyRegion.imageOffset = { .x = 0, .y = 0, .z = 0 };
    copyRegion.imageExtent = { .width = m_width,
                               .height = m_height,
                               .depth = m_depth };

    const auto *vulkanBuffer =
        dynamic_cast<const VulkanBuffer *>(&stagingBuffer);

    if (vulkanBuffer == nullptr) {
        return std::unexpected{ "Staging buffer is not a VulkanBuffer" };
    }

    vkCmdCopyBufferToImage(
        m_frameSync->getTransferCommandBuffer(),
        vulkanBuffer->getBuffer(),
        m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion
    );

    auto finalTransitionResult = transitionLayoutInternal(
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        aspectMask,
        true
    );

    if (!finalTransitionResult) {
        return std::unexpected{ std::format(
            "Failed to transition to final layout: {}",
            finalTransitionResult.error()
        ) };
    }

    auto endResult = m_frameSync->endTransferCommandBuffer();
    if (!endResult) {
        return std::unexpected{ std::format(
            "Failed to end transfer command buffer: {}",
            endResult.error()
        ) };
    }

    auto submitResult = m_frameSync->submitTransferCommandBuffer();
    if (!submitResult) {
        return std::unexpected{ std::format(
            "Failed to submit transfer command buffer: {}",
            submitResult.error()
        ) };
    }

    auto waitResult = m_frameSync->waitForTransferComplete();
    if (!waitResult) {
        return std::unexpected{ std::format(
            "Failed to wait for transfer completion: {}",
            waitResult.error()
        ) };
    }

    m_currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    Logger::debug(
        "Buffer transfer completed: {}x{}x{}",
        m_width,
        m_height,
        m_depth
    );

    return {};
}

} // namespace vostok::graphics::vulkan