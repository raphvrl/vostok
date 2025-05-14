#include "graphics/vulkan/vulkan_device.hpp"

#include "graphics/gpu_device.hpp"
#include "graphics/vulkan/core/device.hpp"
#include "graphics/vulkan/core/frame_sync.hpp"
#include "graphics/vulkan/core/instance.hpp"
#include "graphics/vulkan/core/physical_device.hpp"
#include "graphics/vulkan/core/surface.hpp"
#include "graphics/vulkan/core/swapchain.hpp"
#include "graphics/vulkan/platform/glfw_platform.hpp"
#include "graphics/vulkan/utils/vk_utils.hpp"
#include "volk.h"

#include <memory>

namespace vostok::graphics::vulkan
{

class VulkanDevice::Impl
{
public:
    Impl(const GPUDevice::CreateInfo &createInfo);
    ~Impl() = default;

    Impl(const Impl &) = delete;
    Impl &operator=(const Impl &) = delete;
    Impl(Impl &&) = delete;
    Impl &operator=(Impl &&) = delete;

    void waitIdle();

    std::expected<u32, std::string> beginFrame();
    std::expected<void, std::string> endFrame();

    std::expected<void, std::string> resize(const FramebufferSize &size);

    [[nodiscard]] bool isInitialized() const { return m_instance != nullptr; }

    [[nodiscard]] const std::string &getLastError() const { return m_lastError; }

private:
    bool initInstance(const GPUDevice::CreateInfo &createInfo);
    bool initSurface(void *windowHandle);
    bool initPhysicalDevice();
    bool initDevice(const GPUDevice::CreateInfo &createInfo);
    bool initSwapchain(const SwapchainExtent &size);
    bool initFrameSync();

    std::unique_ptr<Instance> m_instance;
    std::unique_ptr<Surface> m_surface;
    std::unique_ptr<PhysicalDevice> m_physicalDevice;
    std::unique_ptr<Device> m_device;
    std::unique_ptr<Swapchain> m_swapchain;
    std::unique_ptr<FrameSync> m_frameSync;

    std::string m_lastError;

    u32 m_currentImageIndex = 0;
};

std::expected<std::unique_ptr<GPUDevice>, std::string>
VulkanDevice::create(const CreateInfo &createInfo)
{
    auto device = Factory::create();

    device->m_impl = std::make_unique<Impl>(createInfo);

    if (!device->m_impl->isInitialized()) {
        return std::unexpected(device->m_impl->getLastError());
    }

    return device;
}

VulkanDevice::Impl::Impl(const GPUDevice::CreateInfo &createInfo)
{
    if (!initInstance(createInfo)) {
        return;
    }

    if (!initSurface(createInfo.windowHandle)) {
        return;
    }

    if (!initPhysicalDevice()) {
        return;
    }

    if (!initDevice(createInfo)) {
        return;
    }

    if (!initSwapchain({ .width = createInfo.width, .height = createInfo.height })) {
        return;
    }

    if (!initFrameSync()) {
        return;
    }
}

bool VulkanDevice::Impl::initInstance(const GPUDevice::CreateInfo &createInfo)
{
    auto platform = std::make_unique<platform::GlfwPlatform>();

    Instance::CreateInfo instanceCreateInfo;
    instanceCreateInfo.appName = createInfo.appName;
    instanceCreateInfo.appVersion = createInfo.appVersion.toPackedInt();
    instanceCreateInfo.engineName = createInfo.engineName;
    instanceCreateInfo.engineVersion = createInfo.engineVersion.toPackedInt();
    instanceCreateInfo.enableValidationLayers = createInfo.enableValidationLayers;
    instanceCreateInfo.platform = std::move(platform);

    auto result = Instance::create(instanceCreateInfo);
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_instance = std::move(result.value());

    return true;
}

bool VulkanDevice::Impl::initSurface(void *windowHandle)
{
    if (!m_instance) {
        m_lastError = "Instance is not initialized";
        return false;
    }

    auto result = Surface::create(m_instance.get(), windowHandle);
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_surface = std::move(result.value());

    return true;
}

bool VulkanDevice::Impl::initPhysicalDevice()
{
    auto result = PhysicalDevice::create(m_instance->getHandle(), m_surface->getHandle());
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_physicalDevice = std::move(result.value());

    return true;
}

bool VulkanDevice::Impl::initDevice(const GPUDevice::CreateInfo &createInfo)
{
    Device::CreateInfo deviceInfo;
    deviceInfo.physicalDevice = m_physicalDevice.get();
    deviceInfo.surface = m_surface->getHandle();

    deviceInfo.extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
    };

    VkPhysicalDeviceVulkan12Features features12 = {};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.timelineSemaphore = VK_TRUE;
    features12.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;
    features13.pNext = &features12;

    VkPhysicalDeviceVulkan14Features features14 = {};
    features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
    features14.maintenance6 = VK_TRUE;
    features14.pNext = &features13;

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &features14;
    deviceFeatures2.features.samplerAnisotropy = VK_TRUE;

    deviceInfo.pNext = &deviceFeatures2;
    deviceInfo.enableValidationLayers = createInfo.enableValidationLayers;

    auto result = Device::create(deviceInfo);
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_device = std::move(result.value());

    return true;
}

bool VulkanDevice::Impl::initSwapchain(const SwapchainExtent &size)
{
    if (!m_device) {
        m_lastError = "Device is not initialized";
        return false;
    }

    Swapchain::CreateInfo createInfo;
    createInfo.device = m_device.get();
    createInfo.surface = m_surface->getHandle();
    createInfo.width = size.width;
    createInfo.height = size.height;

    auto result = Swapchain::create(createInfo);
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_swapchain = std::move(result.value());

    return true;
}

bool VulkanDevice::Impl::initFrameSync()
{
    if (!m_device) {
        m_lastError = "Device is not initialized";
        return false;
    }

    FrameSync::CreateInfo createInfo;
    createInfo.device = m_device.get();
    createInfo.maxFramesInFlight = 2;
    createInfo.commandPool = m_device->getCommandPool();

    auto result = FrameSync::create(createInfo);
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_frameSync = std::move(result.value());

    return true;
}

void VulkanDevice::Impl::waitIdle()
{
    if (!m_device) {
        return;
    }

    m_device->waitIdle();
}

std::expected<u32, std::string> VulkanDevice::Impl::beginFrame()
{
    if (!m_device || !m_swapchain || !m_frameSync) {
        return std::unexpected("Instance is not initialized");
    }

    m_frameSync->waitForFence();
    m_frameSync->resetFences();

    auto imageResult =
        m_swapchain->acquireNextImage(m_frameSync->getImageAvailableSemaphore(), VK_NULL_HANDLE);

    if (!imageResult) {
        return std::unexpected(imageResult.error());
    }

    u32 imageIndex = imageResult.value();
    m_currentImageIndex = imageIndex;

    auto cmdBufferResult = m_frameSync->beginCommandBuffer();
    if (!cmdBufferResult) {
        return std::unexpected(cmdBufferResult.error());
    }

    VkImageMemoryBarrier2 imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    imageBarrier.srcAccessMask = 0;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageBarrier.image = m_swapchain->getImage(imageIndex);
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependencyInfo = {};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(m_frameSync->getCommandBuffer(), &dependencyInfo);

    VkRenderingAttachmentInfo colorAttachment = {};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = m_swapchain->getImageView(imageIndex);
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = { { 0.0F, 0.0F, 0.0F, 1.0F } };

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = { .x = 0, .y = 0 };
    renderingInfo.renderArea.extent = m_swapchain->getExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(m_frameSync->getCommandBuffer(), &renderingInfo);

    return {};
}

std::expected<void, std::string> VulkanDevice::Impl::endFrame()
{
    if (!m_device || !m_swapchain || !m_frameSync) {
        return std::unexpected("Instance is not initialized");
    }

    vkCmdEndRenderingKHR(m_frameSync->getCommandBuffer());

    VkImageMemoryBarrier2 imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    imageBarrier.dstAccessMask = 0;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    imageBarrier.image = m_swapchain->getImage(m_currentImageIndex);
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependencyInfo = {};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(m_frameSync->getCommandBuffer(), &dependencyInfo);

    auto endResult = m_frameSync->endCommandBuffer();
    if (!endResult) {
        return std::unexpected(endResult.error());
    }

    VkCommandBufferSubmitInfo cmdBufferInfo = {};
    cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmdBufferInfo.commandBuffer = m_frameSync->getCommandBuffer();

    VkSemaphore imageAvailableSemaphore = m_frameSync->getImageAvailableSemaphore();
    VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.semaphore = imageAvailableSemaphore;
    waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphore renderFinishedSemaphore = m_frameSync->getRenderFinishedSemaphore();
    VkSemaphoreSubmitInfo signalSemaphoreInfo = {};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfo.semaphore = renderFinishedSemaphore;
    signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo2 submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &cmdBufferInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

    VkResult result = vkQueueSubmit2(
        m_device->getGraphicsQueue(),
        1,
        &submitInfo,
        m_frameSync->getInFlightFence()
    );

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to submit command buffer: " + utils::vkResultToString(result)
        );
    }

    VkSwapchainKHR swapchain = m_swapchain->getHandle();

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &m_currentImageIndex;

    result = vkQueuePresentKHR(m_device->getPresentQueue(), &presentInfo);
    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to present image: " + utils::vkResultToString(result));
    }

    m_frameSync->nextFrame();

    return {};
}

std::expected<void, std::string> VulkanDevice::Impl::resize(const FramebufferSize &size)
{
    if (size.width == 0 || size.height == 0) {
        return std::unexpected("Invalid framebuffer size");
    }

    if (!m_instance) {
        return std::unexpected("Instance is not initialized");
    }

    // TODO: Implement resize
    return std::unexpected("Not implemented");
}

VulkanDevice::VulkanDevice()
    : m_impl(nullptr)
{}

VulkanDevice::~VulkanDevice()
{
    if (m_impl) {
        m_impl->waitIdle();
    }
}

void VulkanDevice::waitIdle()
{
    if (m_impl) {
        m_impl->waitIdle();
    }
}

std::expected<u32, std::string> VulkanDevice::beginFrame()
{
    if (m_impl) {
        return m_impl->beginFrame();
    }
    return std::unexpected("VulkanDevice is not initialized");
}

std::expected<void, std::string> VulkanDevice::endFrame()
{
    if (m_impl) {
        return m_impl->endFrame();
    }
    return std::unexpected("VulkanDevice is not initialized");
}

std::expected<void, std::string> VulkanDevice::resize(const FramebufferSize &size)
{
    if (m_impl) {
        return m_impl->resize(size);
    }
    return std::unexpected("VulkanDevice is not initialized");
}

} // namespace vostok::graphics::vulkan