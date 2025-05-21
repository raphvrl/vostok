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

class VulkanDevice : public graphics::GPUDevice
{
public:
    static std::expected<std::unique_ptr<GPUDevice>, std::string>
    create(const CreateInfo &createInfo);

    ~VulkanDevice() override;

    VulkanDevice(const VulkanDevice &) = delete;
    VulkanDevice &operator=(const VulkanDevice &) = delete;
    VulkanDevice(VulkanDevice &&) = delete;
    VulkanDevice &operator=(VulkanDevice &&) = delete;

    void waitIdle() override;

    std::expected<u32, std::string> beginFrame() override;
    std::expected<void, std::string> endFrame() override;

    std::expected<void, std::string> resize(const FramebufferSize &size) override;

    void draw(
        u32 vertexCount,
        u32 instanceCount = 1,
        u32 firstVertex = 0,
        u32 firstInstance = 0
    ) override;

    std::expected<std::unique_ptr<Pipeline::Builder>, std::string> createPipelineBuilder() override;

    [[nodiscard]] Instance *getInstance() const;
    [[nodiscard]] Surface *getSurface() const;
    [[nodiscard]] PhysicalDevice *getPhysicalDevice() const;
    [[nodiscard]] Device *getDevice() const;
    [[nodiscard]] Swapchain *getSwapchain() const;
    [[nodiscard]] FrameSync *getFrameSync() const;

private:
    VulkanDevice();

    struct Factory
    {
        static std::unique_ptr<VulkanDevice> create()
        {
            return std::unique_ptr<VulkanDevice>(new VulkanDevice());
        }
    };

    friend struct Factory;

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vostok::graphics::vulkan