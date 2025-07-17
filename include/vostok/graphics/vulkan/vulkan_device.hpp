#pragma once

#include "vostok/graphics/gpu_device.hpp"
#include "vostok/graphics/vulkan/vulkan_pipeline.hpp"

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

class Buffer;

class VulkanGPUDevice : public graphics::GPUDevice
{
public:
    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<GPUDevice>, std::string>;

    ~VulkanGPUDevice() override;

    VulkanGPUDevice(const VulkanGPUDevice &) = delete;
    auto operator=(const VulkanGPUDevice &) -> VulkanGPUDevice & = delete;
    VulkanGPUDevice(VulkanGPUDevice &&) = delete;
    auto operator=(VulkanGPUDevice &&) -> VulkanGPUDevice & = delete;

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

    auto createPipelineBuilder() -> std::
        expected<std::unique_ptr<Pipeline::Builder>, std::string> override;

    [[nodiscard]] auto getInstance() const -> VulkanInstance *;
    [[nodiscard]] auto getSurface() const -> VulkanSurface *;
    [[nodiscard]] auto getPhysicalDevice() const -> VulkanPhysicalDevice *;
    [[nodiscard]] auto getDevice() const -> VulkanDevice *;
    [[nodiscard]] auto getAllocator() const -> VulkanAllocator *;
    [[nodiscard]] auto getSwapchain() const -> VulkanSwapchain *;
    [[nodiscard]] auto getFrameSync() const -> VulkanFrameSync *;

    auto createBuffer(
        const graphics::BufferCreateInfo &createInfo
    ) -> std::expected<std::unique_ptr<graphics::Buffer>, std::string> override;

private:
    VulkanGPUDevice();

    struct Factory
    {
        static auto create() -> std::unique_ptr<VulkanGPUDevice>
        {
            return std::unique_ptr<VulkanGPUDevice>(new VulkanGPUDevice());
        }
    };

    friend struct Factory;

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vostok::graphics::vulkan