#pragma once

#include "graphics/gpu_device.hpp"

#include <expected>
#include <memory>

namespace vostok::graphics::vulkan
{

class VulkanDevice : public GPUDevice
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

    std::expected<void, std::string> resize(FramebufferSize size) override;

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