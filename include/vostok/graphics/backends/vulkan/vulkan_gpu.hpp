#pragma once

#include "vostok/graphics/backends/vulkan/core/vulkan_swapchain.hpp"
#include "vostok/graphics/backends/vulkan/vulkan_pipeline.hpp"
#include "vostok/graphics/gpu.hpp"

#include <expected>
#include <memory>

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

    auto beginFrame() -> std::expected<u32, std::string> override;
    auto endFrame() -> std::expected<void, std::string> override;

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

    [[nodiscard]] auto getInstance() const -> VulkanInstance *;
    [[nodiscard]] auto getSurface() const -> VulkanSurface *;
    [[nodiscard]] auto getPhysicalDevice() const -> VulkanPhysicalDevice *;
    [[nodiscard]] auto getDevice() const -> VulkanDevice *;
    [[nodiscard]] auto getAllocator() const -> VulkanAllocator *;
    [[nodiscard]] auto getSwapchain() const -> VulkanSwapchain *;
    [[nodiscard]] auto getFrameSync() const -> VulkanFrameSync *;
    [[nodiscard]] auto getBindlessManager() const -> VulkanBindlessManager *;

    auto createPipeline(const Pipeline::CreateInfo &createInfo)
        -> std::expected<Pipeline, std::string> override;

    auto createBuffer(
        const graphics::BufferCreateInfo &createInfo
    ) -> std::expected<std::unique_ptr<graphics::Buffer>, std::string> override;

    auto createImage(
        const graphics::ImageCreateInfo &createInfo
    ) -> std::expected<std::unique_ptr<graphics::Image>, std::string> override;

    // Méthodes utilitaires
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

    void notifyDirtyResource(u32 bindlessIndex) override;

    // Méthodes d'initialisation privées
    auto initInstance(const GPUHandle::CreateInfo &createInfo) -> bool;
    auto initSurface(void *windowHandle) -> bool;
    auto initPhysicalDevice() -> bool;
    auto initDevice(const GPUHandle::CreateInfo &createInfo) -> bool;
    auto initAllocator() -> bool;
    auto initSwapchain(const SwapchainExtent &size) -> bool;
    auto initFrameSync() -> bool;
    auto initBindlessManager() -> bool;

    // Membres privés
    std::unique_ptr<VulkanInstance> m_instance;
    std::unique_ptr<VulkanSurface> m_surface;
    std::unique_ptr<VulkanPhysicalDevice> m_physicalDevice;
    std::unique_ptr<VulkanDevice> m_device;
    std::unique_ptr<VulkanAllocator> m_allocator;
    std::unique_ptr<VulkanSwapchain> m_swapchain;
    std::unique_ptr<VulkanFrameSync> m_frameSync;
    std::unique_ptr<VulkanCommandPool> m_graphicsCommandPool;
    std::unique_ptr<VulkanCommandPool> m_transferCommandPool;
    std::unique_ptr<VulkanBindlessManager> m_bindlessManager;
    std::string m_lastError;

    u32 m_currentImageIndex = 0;
};

} // namespace vostok::graphics::vulkan