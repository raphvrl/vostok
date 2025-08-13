#pragma once

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

    [[nodiscard]] auto getInstance() const -> VulkanInstance *;
    [[nodiscard]] auto getSurface() const -> VulkanSurface *;
    [[nodiscard]] auto getPhysicalDevice() const -> VulkanPhysicalDevice *;
    [[nodiscard]] auto getDevice() const -> VulkanDevice *;
    [[nodiscard]] auto getAllocator() const -> VulkanAllocator *;
    [[nodiscard]] auto getSwapchain() const -> VulkanSwapchain *;
    [[nodiscard]] auto getFrameSync() const -> VulkanFrameSync *;
    [[nodiscard]] auto getBindlessManager() const -> VulkanBindlessManager *;

    auto createPipeline(const PipelineCreateInfo &createInfo)
        -> std::expected<std::unique_ptr<PipelineHandle>, std::string> override;

    auto createBuffer(
        const graphics::BufferCreateInfo &createInfo
    ) -> std::expected<std::unique_ptr<graphics::Buffer>, std::string> override;

private:
    VulkanGPU();

    struct Factory
    {
        static auto create() -> std::unique_ptr<VulkanGPU>
        {
            return std::unique_ptr<VulkanGPU>(new VulkanGPU());
        }
    };

    friend struct Factory;

    class Impl;
    std::unique_ptr<Impl> m_impl;

    auto registerUBO(BindableResource *ubo, size_t size)
        -> std::expected<u32, std::string> override;

    void notifyDirtyResource(u32 bindlessIndex) override;
};

} // namespace vostok::graphics::vulkan