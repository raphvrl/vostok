#include "graphics/vulkan/vulkan_device.hpp"

#include "core/logger/logger.hpp"
#include "graphics/gpu_device.hpp"
#include "graphics/vulkan/core/allocator.hpp"
#include "graphics/vulkan/core/command_pool.hpp"
#include "graphics/vulkan/core/device.hpp"
#include "graphics/vulkan/core/frame_sync.hpp"
#include "graphics/vulkan/core/instance.hpp"
#include "graphics/vulkan/core/physical_device.hpp"
#include "graphics/vulkan/core/surface.hpp"
#include "graphics/vulkan/core/swapchain.hpp"
#include "graphics/vulkan/platform/glfw_platform.hpp"
#include "graphics/vulkan/resources/buffer.hpp"
#include "graphics/vulkan/utils/vk_utils.hpp"
#include "graphics/vulkan/vulkan_pipeline.hpp"
#include "volk.h"

#include <expected>
#include <memory>
#include <string>

namespace vostok::graphics::vulkan
{

class VulkanGPUDevice::Impl
{
public:
    Impl(VulkanGPUDevice *parent, const GPUDevice::CreateInfo &createInfo);
    ~Impl();

    Impl(const Impl &) = delete;
    auto operator=(const Impl &) -> Impl & = delete;
    Impl(Impl &&) = delete;
    auto operator=(Impl &&) -> Impl & = delete;

    void waitIdle();

    auto beginFrame() -> std::expected<u32, std::string>;
    auto endFrame() -> std::expected<void, std::string>;

    auto resize(const FramebufferSize &size)
        -> std::expected<void, std::string>;

    void draw(
        u32 vertexCount,
        u32 instanceCount = 1,
        u32 firstVertex = 0,
        u32 firstInstance = 0
    );

    [[nodiscard]] auto isInitialized() const -> bool
    {
        return m_instance && m_surface && m_physicalDevice && m_device &&
               m_allocator && m_swapchain && m_frameSync;
    }

    [[nodiscard]] auto getLastError() const -> const std::string &
    {
        return m_lastError;
    }

    [[nodiscard]] auto getInstance() const -> Instance *
    {
        return m_instance.get();
    }
    [[nodiscard]] auto getSurface() const -> Surface *
    {
        return m_surface.get();
    }
    [[nodiscard]] auto getPhysicalDevice() const -> PhysicalDevice *
    {
        return m_physicalDevice.get();
    }
    [[nodiscard]] auto getDevice() const -> Device * { return m_device.get(); }
    [[nodiscard]] auto getAllocator() const -> Allocator *
    {
        return m_allocator.get();
    }
    [[nodiscard]] auto getSwapchain() const -> Swapchain *
    {
        return m_swapchain.get();
    }
    [[nodiscard]] auto getFrameSync() const -> FrameSync *
    {
        return m_frameSync.get();
    }

    auto createPipelineBuilder()
        -> std::expected<std::unique_ptr<Pipeline::Builder>, std::string>;
    auto createBuffer(const graphics::BufferCreateInfo &createInfo)
        -> std::expected<std::unique_ptr<graphics::Buffer>, std::string>;

private:
    auto initInstance(const GPUDevice::CreateInfo &createInfo) -> bool;
    auto initSurface(void *windowHandle) -> bool;
    auto initPhysicalDevice() -> bool;
    auto initDevice(const GPUDevice::CreateInfo &createInfo) -> bool;
    auto initAllocator() -> bool;
    auto initSwapchain(const SwapchainExtent &size) -> bool;
    auto initFrameSync() -> bool;

    std::unique_ptr<Instance> m_instance;
    std::unique_ptr<Surface> m_surface;
    std::unique_ptr<PhysicalDevice> m_physicalDevice;
    std::unique_ptr<Device> m_device;
    std::unique_ptr<Allocator> m_allocator;
    std::unique_ptr<Swapchain> m_swapchain;
    std::unique_ptr<FrameSync> m_frameSync;
    std::unique_ptr<CommandPool> m_graphicsCommandPool;
    std::unique_ptr<CommandPool> m_transferCommandPool;
    std::string m_lastError;

    u32 m_currentImageIndex = 0;

    VulkanGPUDevice *m_parent;
};

auto VulkanGPUDevice::create(const CreateInfo &createInfo)
    -> std::expected<std::unique_ptr<GPUDevice>, std::string>
{
    auto device = Factory::create();

    device->m_impl = std::make_unique<Impl>(device.get(), createInfo);

    if (!device->m_impl->isInitialized()) {
        return std::unexpected(device->m_impl->getLastError());
    }

    return device;
}

VulkanGPUDevice::Impl::Impl(
    VulkanGPUDevice *parent,
    const GPUDevice::CreateInfo &createInfo
)
    : m_parent(parent)
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

    if (!initAllocator()) {
        return;
    }

    if (!initSwapchain(
            { .width = createInfo.width, .height = createInfo.height }
        )) {
        return;
    }

    if (!initFrameSync()) {
        return;
    }
}

VulkanGPUDevice::Impl::~Impl()
{
    Logger::debug("Vulkan GPU device destructor called");

    if (m_frameSync) {
        Logger::debug("Destroying FrameSync");
        m_frameSync.reset();
    }

    if (m_transferCommandPool) {
        Logger::debug("Destroying transfer command pool");
        m_transferCommandPool.reset();
    }

    if (m_graphicsCommandPool) {
        Logger::debug("Destroying graphics command pool");
        m_graphicsCommandPool.reset();
    }

    if (m_swapchain) {
        Logger::debug("Destroying swapchain");
        m_swapchain.reset();
    }

    if (m_allocator) {
        Logger::debug("Destroying allocator");
        m_allocator.reset();
    }

    if (m_device) {
        Logger::debug("Destroying device");
        m_device.reset();
    }

    if (m_physicalDevice) {
        Logger::debug("Destroying physical device");
        m_physicalDevice.reset();
    }

    if (m_surface) {
        Logger::debug("Destroying surface");
        m_surface.reset();
    }

    if (m_instance) {
        Logger::debug("Destroying instance");
        m_instance.reset();
    }

    Logger::debug("Vulkan GPU device destructor completed");
}

auto VulkanGPUDevice::Impl::initInstance(
    const GPUDevice::CreateInfo &createInfo
) -> bool
{
    auto platform = std::make_unique<platform::GlfwPlatform>();

    Instance::CreateInfo instanceCreateInfo;
    instanceCreateInfo.appName = createInfo.appName;
    instanceCreateInfo.appVersion = createInfo.appVersion.toPackedInt();
    instanceCreateInfo.engineName = createInfo.engineName;
    instanceCreateInfo.engineVersion = createInfo.engineVersion.toPackedInt();
    instanceCreateInfo.enableValidationLayers =
        createInfo.enableValidationLayers;
    instanceCreateInfo.platform = std::move(platform);

    auto result = Instance::create(instanceCreateInfo);
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_instance = std::move(result.value());

    return true;
}

auto VulkanGPUDevice::Impl::initSurface(void *windowHandle) -> bool
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

auto VulkanGPUDevice::Impl::initPhysicalDevice() -> bool
{
    auto result =
        PhysicalDevice::create(m_instance->getHandle(), m_surface->getHandle());
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_physicalDevice = std::move(result.value());

    return true;
}

auto VulkanGPUDevice::Impl::initDevice(const GPUDevice::CreateInfo &createInfo)
    -> bool
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

auto VulkanGPUDevice::Impl::initAllocator() -> bool
{
    if (!m_device) {
        m_lastError = "Device is not initialized";
        return false;
    }

    Allocator::CreateInfo allocatorInfo;
    allocatorInfo.instance = m_instance.get();
    allocatorInfo.physicalDevice = m_physicalDevice.get();
    allocatorInfo.device = m_device.get();

    auto result = Allocator::create(allocatorInfo);
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_allocator = std::move(result.value());

    return true;
}

auto VulkanGPUDevice::Impl::initSwapchain(const SwapchainExtent &size) -> bool
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

auto VulkanGPUDevice::Impl::initFrameSync() -> bool
{
    if (!m_device) {
        m_lastError = "Device is not initialized";
        return false;
    }

    auto *physicalDevice = m_device->getPhysicalDevice();
    auto queueFamilyIndices = physicalDevice->getQueueFamilyIndices();

    auto graphicsPoolResult = CommandPool::create(
        { .device = m_device.get(),
          .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
          .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
          .maxFramesInFlight = 2 }
    );

    if (!graphicsPoolResult) {
        m_lastError = graphicsPoolResult.error();
        return false;
    }

    m_graphicsCommandPool = std::move(graphicsPoolResult.value());

    auto transferPoolResult = CommandPool::create(
        { .device = m_device.get(),
          .queueFamilyIndex = queueFamilyIndices.transferFamily.value(),
          .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
          .maxFramesInFlight = 2 }
    );

    if (!transferPoolResult) {
        m_lastError = transferPoolResult.error();
        return false;
    }

    m_transferCommandPool = std::move(transferPoolResult.value());

    FrameSync::CreateInfo createInfo;
    createInfo.device = m_device.get();
    createInfo.maxFramesInFlight = 2;
    createInfo.commandPools.graphics = m_graphicsCommandPool.get();
    createInfo.commandPools.transfer = m_transferCommandPool.get();
    createInfo.transferQueue = m_device->getTransferQueue();

    auto frameSyncResult = FrameSync::create(createInfo);
    if (!frameSyncResult) {
        m_lastError = frameSyncResult.error();
        return false;
    }

    m_frameSync = std::move(frameSyncResult.value());

    return true;
}

void VulkanGPUDevice::Impl::waitIdle()
{
    if (!m_device) {
        return;
    }

    m_device->waitIdle();
}

auto VulkanGPUDevice::Impl::beginFrame() -> std::expected<u32, std::string>
{
    if (!m_device || !m_swapchain || !m_frameSync) {
        return std::unexpected("Instance is not initialized");
    }

    m_frameSync->waitForFence();
    m_frameSync->resetFences();

    auto imageResult = m_swapchain->acquireNextImage(
        m_frameSync->getImageAvailableSemaphore(),
        VK_NULL_HANDLE
    );

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

    VkViewport viewport = {};
    viewport.x = 0.0F;
    viewport.y = 0.0F;
    viewport.width = static_cast<float>(m_swapchain->getExtent().width);
    viewport.height = static_cast<float>(m_swapchain->getExtent().height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    vkCmdSetViewport(m_frameSync->getCommandBuffer(), 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = { .x = 0, .y = 0 };
    scissor.extent = m_swapchain->getExtent();
    vkCmdSetScissor(m_frameSync->getCommandBuffer(), 0, 1, &scissor);

    return {};
}

auto VulkanGPUDevice::Impl::endFrame() -> std::expected<void, std::string>
{
    if (!m_device || !m_swapchain || !m_frameSync) {
        return std::unexpected("Instance is not initialized");
    }

    vkCmdEndRendering(m_frameSync->getCommandBuffer());

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

    VkSemaphore imageAvailableSemaphore =
        m_frameSync->getImageAvailableSemaphore();
    VkSemaphoreSubmitInfo waitSemaphoreInfo = {};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.semaphore = imageAvailableSemaphore;
    waitSemaphoreInfo.stageMask =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphore renderFinishedSemaphore =
        m_frameSync->getRenderFinishedSemaphore();
    VkSemaphoreSubmitInfo signalSemaphoreInfo = {};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfo.semaphore = renderFinishedSemaphore;
    signalSemaphoreInfo.stageMask =
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

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
            "Failed to submit command buffer: " +
            utils::vkResultToString(result)
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
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return std::unexpected("Swapchain is out of date");
    }

    if (result == VK_SUBOPTIMAL_KHR) {
        Logger::warning("Swapchain is suboptimal during present");
        // Continue with suboptimal swapchain
    }

    if (result == VK_ERROR_SURFACE_LOST_KHR) {
        return std::unexpected("Surface lost during present");
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        return std::unexpected(
            "Failed to present image: " + utils::vkResultToString(result)
        );
    }

    m_frameSync->nextFrame();

    return {};
}

auto VulkanGPUDevice::Impl::resize(const FramebufferSize &size)
    -> std::expected<void, std::string>
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

void VulkanGPUDevice::Impl::draw(
    u32 vertexCount,
    u32 instanceCount,
    u32 firstVertex,
    u32 firstInstance
)
{
    if (!m_device || !m_frameSync) {
        return;
    }

    m_frameSync
        ->cmdDraw(vertexCount, instanceCount, firstVertex, firstInstance);
}

auto VulkanGPUDevice::Impl::createPipelineBuilder()
    -> std::expected<std::unique_ptr<Pipeline::Builder>, std::string>
{
    if (!m_device) {
        return std::unexpected("Device is not initialized");
    }

    return VulkanPipeline::Builder::create(m_parent);
}

auto VulkanGPUDevice::Impl::createBuffer(
    const graphics::BufferCreateInfo &createInfo
) -> std::expected<std::unique_ptr<graphics::Buffer>, std::string>
{
    if (!m_allocator) {
        return std::unexpected("Allocator is not initialized");
    }

    if (createInfo.size == 0) {
        return std::unexpected("Buffer size must be greater than 0");
    }

    auto vulkanBuffer = VulkanBuffer::create(m_parent, createInfo);
    if (!vulkanBuffer) {
        return std::unexpected("Failed to create Vulkan buffer");
    }

    return std::unique_ptr<graphics::Buffer>(vulkanBuffer.release());
}

VulkanGPUDevice::VulkanGPUDevice()
    : m_impl(nullptr)
{}

VulkanGPUDevice::~VulkanGPUDevice()
{
    if (m_impl) {
        m_impl->waitIdle();
    }
}

void VulkanGPUDevice::waitIdle()
{
    if (m_impl) {
        m_impl->waitIdle();
    }
}

auto VulkanGPUDevice::beginFrame() -> std::expected<u32, std::string>
{
    if (m_impl) {
        return m_impl->beginFrame();
    }
    return std::unexpected("Vulkan GPU device is not initialized");
}

auto VulkanGPUDevice::endFrame() -> std::expected<void, std::string>
{
    if (m_impl) {
        return m_impl->endFrame();
    }
    return std::unexpected("Vulkan GPU device is not initialized");
}

auto VulkanGPUDevice::resize(const FramebufferSize &size)
    -> std::expected<void, std::string>
{
    if (m_impl) {
        return m_impl->resize(size);
    }
    return std::unexpected("Vulkan GPU device is not initialized");
}

void VulkanGPUDevice::draw(
    u32 vertexCount,
    u32 instanceCount,
    u32 firstVertex,
    u32 firstInstance
)
{
    if (m_impl) {
        m_impl->draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }
}

auto VulkanGPUDevice::createPipelineBuilder()
    -> std::expected<std::unique_ptr<Pipeline::Builder>, std::string>
{
    if (m_impl) {
        return m_impl->createPipelineBuilder();
    }
    return std::unexpected("Vulkan GPU device is not initialized");
}

auto VulkanGPUDevice::createBuffer(const graphics::BufferCreateInfo &createInfo)
    -> std::expected<std::unique_ptr<graphics::Buffer>, std::string>
{
    if (m_impl) {
        return m_impl->createBuffer(createInfo);
    }

    return std::unexpected("Vulkan GPU device is not initialized");
}

auto VulkanGPUDevice::getInstance() const -> Instance *
{
    if (m_impl) {
        return m_impl->getInstance();
    }
    return nullptr;
}

auto VulkanGPUDevice::getSurface() const -> Surface *
{
    if (m_impl) {
        return m_impl->getSurface();
    }
    return nullptr;
}

auto VulkanGPUDevice::getPhysicalDevice() const -> PhysicalDevice *
{
    if (m_impl) {
        return m_impl->getPhysicalDevice();
    }
    return nullptr;
}

auto VulkanGPUDevice::getDevice() const -> Device *
{
    if (m_impl) {
        return m_impl->getDevice();
    }
    return nullptr;
}

auto VulkanGPUDevice::getAllocator() const -> Allocator *
{
    if (m_impl) {
        return m_impl->getAllocator();
    }
    return nullptr;
}

auto VulkanGPUDevice::getSwapchain() const -> Swapchain *
{
    if (m_impl) {
        return m_impl->getSwapchain();
    }
    return nullptr;
}

auto VulkanGPUDevice::getFrameSync() const -> FrameSync *
{
    if (m_impl) {
        return m_impl->getFrameSync();
    }
    return nullptr;
}

} // namespace vostok::graphics::vulkan