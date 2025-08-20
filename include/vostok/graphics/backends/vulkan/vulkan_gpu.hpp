#pragma once

#include "vostok/graphics/backends/vulkan/core/vulkan_swapchain.hpp"
#include "vostok/graphics/backends/vulkan/vulkan_pipeline.hpp"
#include "vostok/graphics/gpu.hpp"

#include <expected>
#include <memory>
#include <optional>

namespace vostok::graphics::vulkan
{

class VulkanInstance;
class VulkanSurface;
class VulkanPhysicalDevice;
class VulkanDevice;
class VulkanDevice;
class VulkanSwapchain;
class VulkanFrameSync;
class VulkanAllocator;
class VulkanBindlessManager;
class VulkanCommandPool;
class VulkanImage;

class Buffer;

class VulkanGPU : public graphics::GPUHandle
{
public:
    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<GPUHandle>, std::string>;

    ~VulkanGPU() override;

    VulkanGPU(const VulkanGPU &) = delete;
    auto operator=(const VulkanGPU &) -> VulkanGPU & = delete;
    VulkanGPU(VulkanGPU &&) = delete;
    auto operator=(VulkanGPU &&) -> VulkanGPU & = delete;

    void waitIdle() override;

    auto beginFrame() -> std::expected<u32, graphics::FrameErrorInfo> override;
    auto endFrame() -> std::expected<void, graphics::FrameErrorInfo> override;

    auto resize(const FramebufferSize &size)
        -> std::expected<void, std::string> override;

    void draw(
        u32 vertexCount,
        u32 instanceCount = 1,
        u32 firstVertex = 0,
        u32 firstInstance = 0
    ) override;

    void drawIndexed(
        u32 indexCount,
        u32 instanceCount = 1,
        u32 firstIndex = 0,
        u32 vertexOffset = 0,
        u32 firstInstance = 0
    ) override;

    [[nodiscard]] auto getInstance() const -> VulkanInstance *
    {
        return m_instance.get();
    }
    [[nodiscard]] auto getSurface() const -> VulkanSurface *
    {
        return m_surface.get();
    }
    [[nodiscard]] auto getPhysicalDevice() const -> VulkanPhysicalDevice *
    {
        return m_physicalDevice.get();
    }
    [[nodiscard]] auto getDevice() const -> VulkanDevice *
    {
        return m_device.get();
    }
    [[nodiscard]] auto getAllocator() const -> VulkanAllocator *
    {
        return m_allocator.get();
    }
    [[nodiscard]] auto getSwapchain() const -> VulkanSwapchain *
    {
        return m_swapchain.get();
    }
    [[nodiscard]] auto getFrameSync() const -> VulkanFrameSync *
    {
        return m_frameSync.get();
    }
    [[nodiscard]] auto getBindlessManager() const -> VulkanBindlessManager *
    {
        return m_bindlessManager.get();
    }

    [[nodiscard]] auto getDepthImage() const -> VulkanImage *
    {
        return m_depthImage.get();
    }

    auto createPipeline(const Pipeline::CreateInfo &createInfo)
        -> std::expected<Pipeline, std::string> override;

    auto createBuffer(
        const graphics::BufferCreateInfo &createInfo
    ) -> std::expected<std::unique_ptr<graphics::Buffer>, std::string> override;

    auto createImage(
        const graphics::ImageCreateInfo &createInfo
    ) -> std::expected<std::unique_ptr<graphics::Image>, std::string> override;

    [[nodiscard]] auto isInitialized() const -> bool;
    [[nodiscard]] auto getLastError() const -> const std::string &;

private:
    VulkanGPU(const GPUHandle::CreateInfo &createInfo);

    struct Factory
    {
        static auto create(const GPUHandle::CreateInfo &createInfo)
            -> std::unique_ptr<VulkanGPU>
        {
            return std::unique_ptr<VulkanGPU>(new VulkanGPU(createInfo));
        }
    };

    friend struct Factory;

    auto registerUBO(BindableResource *ubo, size_t size)
        -> std::expected<u32, std::string> override;
    auto registerSSBO(BindableResource *ssbo, size_t size)
        -> std::expected<u32, std::string> override;
    auto registerTexture(BindableResource *texture)
        -> std::expected<u32, std::string> override;

    auto unregisterUBO(BindableResource *ubo)
        -> std::expected<void, std::string> override;
    auto unregisterSSBO(BindableResource *ssbo)
        -> std::expected<void, std::string> override;
    auto unregisterTexture(BindableResource *texture)
        -> std::expected<void, std::string> override;

    void notifyDirtyResource(u32 bindlessIndex) override;

    auto initInstance(const GPUHandle::CreateInfo &createInfo) -> bool;
    auto initSurface(void *windowHandle) -> bool;
    auto initPhysicalDevice() -> bool;
    auto initDevice(const GPUHandle::CreateInfo &createInfo) -> bool;
    auto initAllocator() -> bool;
    auto initSwapchain(const SwapchainExtent &size) -> bool;
    auto initFrameSync() -> bool;
    auto initBindlessManager() -> bool;

    std::unique_ptr<VulkanInstance> m_instance;
    std::unique_ptr<VulkanSurface> m_surface;
    std::unique_ptr<VulkanPhysicalDevice> m_physicalDevice;
    std::unique_ptr<VulkanDevice> m_device;
    std::unique_ptr<VulkanAllocator> m_allocator;
    std::unique_ptr<VulkanSwapchain> m_swapchain;
    std::unique_ptr<VulkanCommandPool> m_graphicsCommandPool;
    std::unique_ptr<VulkanCommandPool> m_transferCommandPool;
    std::unique_ptr<VulkanFrameSync> m_frameSync;
    std::unique_ptr<VulkanBindlessManager> m_bindlessManager;
    std::string m_lastError;

    u32 m_currentImageIndex = 0;

    std::unique_ptr<VulkanImage> m_depthImage;
    auto createDepthBuffer(u32 width, u32 height)
        -> std::expected<void, std::string>;
    auto recreateDepthImage(u32 width, u32 height)
        -> std::expected<void, std::string>;

    static auto convertSwapchainErrorToFrameError(
        const graphics::SwapchainErrorInfo &swapchainError,
        const std::string &context
    ) -> graphics::FrameErrorInfo;
};

} // namespace vostok::graphics::vulkan