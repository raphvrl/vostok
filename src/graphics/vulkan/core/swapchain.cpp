#include "graphics/vulkan/core/swapchain.hpp"

#include "core/logger/logger.hpp"
#include "graphics/vulkan/core/physical_device.hpp"
#include "graphics/vulkan/utils/vk_utils.hpp"
#include "utils/stl/optional.inl"

#include <algorithm>
#include <limits>
#include <volk.h>

namespace vu = vostok::utils;

namespace vostok::graphics::vulkan
{

Swapchain::Swapchain(
    Device *device,
    VkSurfaceKHR surface,
    VkSwapchainKHR swapchain,
    VkFormat format,
    VkExtent2D extent
)
    : m_device(device),
      m_surface(surface),
      m_swapchain(swapchain),
      m_format(format),
      m_extent(extent)
{}

Swapchain::~Swapchain()
{
    cleanup();
}

Swapchain::Swapchain(Swapchain &&other) noexcept
    : m_device(other.m_device),
      m_swapchain(other.m_swapchain),
      m_format(other.m_format),
      m_extent(other.m_extent),
      m_images(std::move(other.m_images)),
      m_imageViews(std::move(other.m_imageViews))
{
    other.m_swapchain = VK_NULL_HANDLE;
    other.m_format = VK_FORMAT_UNDEFINED;
    other.m_extent = {};
}

auto Swapchain::operator=(Swapchain &&other) noexcept -> Swapchain &
{
    if (this != &other) {
        cleanup();

        m_device = other.m_device;
        m_swapchain = other.m_swapchain;
        m_format = other.m_format;
        m_extent = other.m_extent;
        m_images = std::move(other.m_images);
        m_imageViews = std::move(other.m_imageViews);

        other.m_swapchain = VK_NULL_HANDLE;
        other.m_format = VK_FORMAT_UNDEFINED;
        other.m_extent = {};
    }
    return *this;
}

void Swapchain::cleanup()
{
    if (m_device == nullptr || m_swapchain == VK_NULL_HANDLE) {
        return;
    }

    for (auto *imageView : m_imageViews) {
        vkDestroyImageView(m_device->getHandle(), imageView, nullptr);
    }

    m_imageViews.clear();

    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device->getHandle(), m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }

    m_images.clear();
    Logger::debug("Swapchain destroyed");
}

auto Swapchain::create(const CreateInfo &createInfo)
    -> std::expected<std::unique_ptr<Swapchain>, std::string>
{
    if (createInfo.device == nullptr) {
        return std::unexpected("Device is null");
    }

    if (createInfo.surface == VK_NULL_HANDLE) {
        return std::unexpected("Surface is null");
    }

    if (createInfo.width == 0 || createInfo.height == 0) {
        return std::unexpected("Width and height must be greater than 0");
    }

    PhysicalDevice *physicalDevice = createInfo.device->getPhysicalDevice();
    VkPhysicalDevice physicalDeviceHandle = physicalDevice->getHandle();
    VkDevice deviceHandle = createInfo.device->getHandle();

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDeviceHandle,
        createInfo.surface,
        &surfaceCapabilities
    );

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to get surface capabilities: " + utils::vkResultToString(result)
        );
    }

    u32 formatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDeviceHandle,
        createInfo.surface,
        &formatCount,
        nullptr
    );

    if (result != VK_SUCCESS || formatCount == 0) {
        return std::unexpected(
            "Failed to get surface formats count: " + utils::vkResultToString(result)
        );
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDeviceHandle,
        createInfo.surface,
        &formatCount,
        surfaceFormats.data()
    );

    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to get surface formats: " + utils::vkResultToString(result));
    }

    u32 presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDeviceHandle,
        createInfo.surface,
        &presentModeCount,
        nullptr
    );

    if (result != VK_SUCCESS || presentModeCount == 0) {
        return std::unexpected(
            "Failed to get surface present modes count: " + utils::vkResultToString(result)
        );
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDeviceHandle,
        createInfo.surface,
        &presentModeCount,
        presentModes.data()
    );

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to get surface present modes: " + utils::vkResultToString(result)
        );
    }

    VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
    for (const auto &availableFormat : surfaceFormats) {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (createInfo.vsync) {
        presentMode = VK_PRESENT_MODE_FIFO_KHR;
    } else {
        for (const auto &availablePresentMode : presentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = availablePresentMode;
                break;
            }
        }
    }

    VkExtent2D extent = {};
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
        extent = surfaceCapabilities.currentExtent;
    } else {
        extent.width = std::clamp(
            createInfo.width,
            surfaceCapabilities.minImageExtent.width,
            surfaceCapabilities.maxImageExtent.width
        );
        extent.height = std::clamp(
            createInfo.height,
            surfaceCapabilities.minImageExtent.height,
            surfaceCapabilities.maxImageExtent.height
        );
    }

    u32 imageCount = surfaceCapabilities.minImageCount + 1;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    if (createInfo.imageCount > 0) {
        imageCount = std::min(imageCount, createInfo.imageCount);
    }

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = createInfo.surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    auto indices = physicalDevice->getQueueFamilyIndices();
    std::array<u32, 2> queueFamilyIndices = { vu::getValueSafe(indices.graphicsFamily, "graphics"),
                                              vu::getValueSafe(indices.presentFamily, "present") };

    if (indices.graphicsFamily != indices.presentFamily) {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = static_cast<u32>(queueFamilyIndices.size());
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    result = vkCreateSwapchainKHR(deviceHandle, &swapchainInfo, nullptr, &swapchain);
    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to create swapchain: " + utils::vkResultToString(result));
    }

    auto swapchainPtr = std::unique_ptr<Swapchain>(new Swapchain(
        createInfo.device,
        createInfo.surface,
        swapchain,
        surfaceFormat.format,
        extent
    ));

    u32 actualImageCount = 0;
    result = vkGetSwapchainImagesKHR(deviceHandle, swapchain, &actualImageCount, nullptr);
    if (result != VK_SUCCESS || actualImageCount == 0) {
        return std::unexpected(
            "Failed to get swapchain images count: " + utils::vkResultToString(result)
        );
    }

    swapchainPtr->m_images.resize(actualImageCount);
    result = vkGetSwapchainImagesKHR(
        deviceHandle,
        swapchain,
        &actualImageCount,
        swapchainPtr->m_images.data()
    );

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to get swapchain images: " + utils::vkResultToString(result)
        );
    }

    if (!swapchainPtr->createImageViews()) {
        return std::unexpected("Failed to create swapchain image views");
    }

    Logger::info("Swapchain created with {} images", swapchainPtr->m_images.size());
    Logger::debug("Swapchain format: {}", utils::vkFormatToString(surfaceFormat.format));
    Logger::debug("Swapchain present mode: {}", utils::vkPresentModeToString(presentMode));
    Logger::debug("Swapchain extent: {}x{}", extent.width, extent.height);

    return swapchainPtr;
}

auto Swapchain::createImageViews() -> bool
{
    m_imageViews.resize(m_images.size());

    for (size_t i = 0; i < m_images.size(); ++i) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkResult result =
            vkCreateImageView(m_device->getHandle(), &viewInfo, nullptr, &m_imageViews[i]);
        if (result != VK_SUCCESS) {
            Logger::error("Failed to create image views: {}", utils::vkResultToString(result));
            return false;
        }
    }

    Logger::debug("Created {} swapchain image views", m_imageViews.size());
    return true;
}

auto Swapchain::acquireNextImage(VkSemaphore semaphore, VkFence fence)
    -> std::expected<u32, std::string>
{
    if (m_swapchain == VK_NULL_HANDLE || m_device == nullptr) {
        return std::unexpected("Swapchain is invalid");
    }

    u32 imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        m_device->getHandle(),
        m_swapchain,
        std::numeric_limits<u64>::max(),
        semaphore,
        fence,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return std::unexpected("Swapchain out of date");
    }

    if (result == VK_SUBOPTIMAL_KHR) {
        Logger::warning("Swapchain is suboptimal");
    }

    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to acquire next image: " + utils::vkResultToString(result));
    }

    return imageIndex;
}

auto Swapchain::present(u32 imageIndex, VkSemaphore renderSemaphore)
    -> std::expected<void, std::string>
{
    if (m_device == nullptr || m_swapchain == VK_NULL_HANDLE) {
        return std::unexpected("Swapchain is invalid");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    if (renderSemaphore != VK_NULL_HANDLE) {
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderSemaphore;
    }

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(m_device->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return std::unexpected("Swapchain is out of date");
    }

    if (result == VK_SUBOPTIMAL_KHR) {
        Logger::warning("Swapchain is suboptimal");
    }

    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to present image: " + utils::vkResultToString(result));
    }

    return {};
}

auto Swapchain::recreate(const SwapchainExtent &size)
    -> std::expected<std::unique_ptr<Swapchain>, std::string>
{
    if (m_device == nullptr) {
        return std::unexpected("Device is null");
    }

    m_device->waitIdle();

    CreateInfo createInfo;
    createInfo.device = m_device;
    createInfo.surface = m_surface;
    createInfo.width = size.width;
    createInfo.height = size.height;
    createInfo.imageCount = static_cast<u32>(m_images.size());

    return create(createInfo);
}

} // namespace vostok::graphics::vulkan