#pragma once

#include "vostok/graphics/pipeline.hpp"

#include <expected>
#include <memory>

struct VkPipeline_T;
struct VkPipelineLayout_T;

namespace vostok::graphics::vulkan
{

class VulkanGPU;

class VulkanPipeline : public Pipeline
{
public:
    VulkanPipeline(
        VulkanGPU *gpu,
        VkPipeline_T *pipeline,
        VkPipelineLayout_T *layout
    );
    ~VulkanPipeline() override;

    VulkanPipeline(const VulkanPipeline &) = delete;
    auto operator=(const VulkanPipeline &) -> VulkanPipeline & = delete;
    VulkanPipeline(VulkanPipeline &&) = delete;
    auto operator=(VulkanPipeline &&) -> VulkanPipeline & = delete;

    static auto create(VulkanGPU *gpu, const PipelineCreateInfo &info)
        -> std::expected<std::unique_ptr<VulkanPipeline>, std::string>;

    void bind() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vostok::graphics::vulkan