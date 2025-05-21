#pragma once

#include "vostok/graphics/pipeline.hpp"

#include <expected>
#include <functional>
#include <memory>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanDevice;

class VulkanPipeline : public Pipeline
{
public:
    ~VulkanPipeline() override;

    VulkanPipeline(const VulkanPipeline &) = delete;
    VulkanPipeline &operator=(const VulkanPipeline &) = delete;
    VulkanPipeline(VulkanPipeline &&) = delete;
    VulkanPipeline &operator=(VulkanPipeline &&) = delete;

    [[nodiscard]] VkPipeline getHandle() const;
    [[nodiscard]] VkPipelineLayout getLayout() const;

    class Builder : public Pipeline::Builder
    {
    public:
        static std::expected<std::unique_ptr<Pipeline::Builder>, std::string>
        create(VulkanDevice *device);

        Builder(VulkanDevice *device);
        ~Builder() override;

        Builder(Builder &) = delete;
        Builder &operator=(const Builder &) = delete;
        Builder(Builder &&) = delete;
        Builder &operator=(Builder &&) = delete;

        Builder &setVertexShader(const fs::path &path) override;
        Builder &setFragmentShader(const fs::path &path) override;
        Builder &setGeometryShader(const fs::path &path) override;
        Builder &setTessellationControlShader(const fs::path &path) override;
        Builder &setTessellationEvaluationShader(const fs::path &path) override;
        Builder &setComputeShader(const fs::path &path) override;

        Builder &setPrimitiveTopology(const PrimitiveTopology &topology) override;

        Builder &setPolygonMode(const PolygonMode &mode) override;
        Builder &setCullMode(const CullMode &mode) override;
        Builder &setFrontFace(const FrontFace &face) override;
        Builder &setLineWidth(f32 width) override;

        Builder &setDepthTest(bool enable) override;
        Builder &setDepthWrite(bool enable) override;
        Builder &setDepthCompareOp(const CompareOp &op) override;
        Builder &setStencilTest(bool enable) override;
        Builder &setStencilOp(
            const StencilOp &failOp,
            const StencilOp &passOp,
            const StencilOp &depthFailOp
        ) override;

        Builder &setBlend(bool enable) override;
        Builder &setBlendFactor(
            const BlendFactor &srcColor,
            const BlendFactor &dstColor,
            const BlendFactor &srcAlpha,
            const BlendFactor &dstAlpha
        ) override;
        Builder &setBlendOp(const BlendOp &colorOp, const BlendOp &alphaOp) override;
        Builder &setColorWriteMask(const ColorComponentFlags &mask) override;

        Builder &addPushConstant(u32 size) override;

        Builder &setName(const std::string &name) override;

        std::expected<std::unique_ptr<Pipeline>, std::string> build() override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

    void bind() override;

private:
    VulkanPipeline(VulkanDevice *device, VkPipeline pipeline, VkPipelineLayout layout);

    struct Factory
    {
        static std::unique_ptr<VulkanPipeline>
        create(VulkanDevice *device, VkPipeline pipeline, VkPipelineLayout layout)
        {
            return std::unique_ptr<VulkanPipeline>(new VulkanPipeline(device, pipeline, layout));
        }
    };

    friend struct Factory;

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vostok::graphics::vulkan