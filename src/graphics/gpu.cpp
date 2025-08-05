#include "graphics/gpu.hpp"

#include "graphics/backends/vulkan/vulkan_gpu.hpp"

namespace vostok::graphics
{

auto GPU::create(const CreateInfo &createInfo, RenderBackend backend)
    -> std::expected<std::unique_ptr<GPU>, std::string>
{
    switch (backend) {
        case RenderBackend::VULKAN:
            return vulkan::VulkanGPU::create(createInfo);
        default:
            return std::unexpected("Unsupported rendering backend");
    }
}

} // namespace vostok::graphics