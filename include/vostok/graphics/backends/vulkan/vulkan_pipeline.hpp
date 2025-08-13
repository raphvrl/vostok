#pragma once

#include "vostok/graphics/pipeline.hpp"

#include <expected>
#include <memory>
#include <span>
#include <vector>

struct VkPipeline_T;
struct VkPipelineLayout_T;
struct VkShaderModule_T;

namespace vostok::graphics::vulkan
{

class VulkanGPU;

class VulkanPipeline : public PipelineHandle
{
public:
    VulkanPipeline(
        VulkanGPU *gpu,
        VkPipeline_T *pipeline,
        VkPipelineLayout_T *layout,
        std::vector<VkShaderModule_T *> shaderModules
    );
    ~VulkanPipeline() override;

    VulkanPipeline(const VulkanPipeline &) = delete;
    auto operator=(const VulkanPipeline &) -> VulkanPipeline & = delete;
    VulkanPipeline(VulkanPipeline &&) = delete;
    auto operator=(VulkanPipeline &&) -> VulkanPipeline & = delete;

    static auto create(VulkanGPU *gpu, const PipelineCreateInfo &info)
        -> std::expected<std::unique_ptr<VulkanPipeline>, std::string>;

    void bind() override;

    auto pushRaw(std::span<const std::byte> data, u32 offset = 0)
        -> std::expected<void, std::string> override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vostok::graphics::vulkan