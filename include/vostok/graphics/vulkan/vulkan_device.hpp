#pragma once

#include "vostok/graphics/gpu_device.hpp"
#include "vostok/graphics/vulkan/vulkan_pipeline.hpp"

#include <expected>
#include <memory>

namespace vostok::graphics::vulkan
{

class Instance;
class Surface;
class PhysicalDevice;
class Device;
class Device;
class Swapchain;
class FrameSync;
class Allocator;

class VulkanDevice : public graphics::GPUDevice
{
public:
    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<GPUDevice>, std::string>;

    ~VulkanDevice() override;

    VulkanDevice(const VulkanDevice &) = delete;
    auto operator=(const VulkanDevice &) -> VulkanDevice & = delete;
    VulkanDevice(VulkanDevice &&) = delete;
    auto operator=(VulkanDevice &&) -> VulkanDevice & = delete;

    void waitIdle() override;

    auto beginFrame() -> std::expected<u32, std::string> override;
    auto endFrame() -> std::expected<void, std::string> override;

    auto resize(const FramebufferSize &size) -> std::expected<void, std::string> override;

    void draw(
        u32 vertexCount,
        u32 instanceCount = 1,
        u32 firstVertex = 0,
        u32 firstInstance = 0
    ) override;

    auto createPipelineBuilder()
        -> std::expected<std::unique_ptr<Pipeline::Builder>, std::string> override;

    [[nodiscard]] auto getInstance() const -> Instance *;
    [[nodiscard]] auto getSurface() const -> Surface *;
    [[nodiscard]] auto getPhysicalDevice() const -> PhysicalDevice *;
    [[nodiscard]] auto getDevice() const -> Device *;
    [[nodiscard]] auto getAllocator() const -> Allocator *;
    [[nodiscard]] auto getSwapchain() const -> Swapchain *;
    [[nodiscard]] auto getFrameSync() const -> FrameSync *;

private:
    VulkanDevice();

    struct Factory
    {
        static auto create() -> std::unique_ptr<VulkanDevice>
        {
            return std::unique_ptr<VulkanDevice>(new VulkanDevice());
        }
    };

    friend struct Factory;

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vostok::graphics::vulkan