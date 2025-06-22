#include "graphics/gpu_device.hpp"

#include "graphics/vulkan/vulkan_device.hpp"

namespace vostok::graphics
{

auto GPUDevice::create(const CreateInfo &createInfo, RenderBackend backend)
    -> std::expected<std::unique_ptr<GPUDevice>, std::string>
{
    switch (backend) {
        case RenderBackend::VULKAN:
            return vulkan::VulkanDevice::create(createInfo);
        default:
            return std::unexpected("Unsupported rendering backend");
    }
}

} // namespace vostok::graphics