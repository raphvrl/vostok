#pragma once

#include "vostok/graphics/pipeline.hpp"

#include <expected>
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
    auto operator=(const VulkanPipeline &) -> VulkanPipeline & = delete;
    VulkanPipeline(VulkanPipeline &&) = delete;
    auto operator=(VulkanPipeline &&) -> VulkanPipeline & = delete;

    [[nodiscard]] auto getHandle() const -> VkPipeline;
    [[nodiscard]] auto getLayout() const -> VkPipelineLayout;

    class Builder : public Pipeline::Builder
    {
    public:
        static auto create(VulkanDevice *device)
            -> std::expected<std::unique_ptr<Pipeline::Builder>, std::string>;

        Builder(VulkanDevice *device);
        ~Builder() override;

        Builder(Builder &) = delete;
        auto operator=(const Builder &) -> Builder & = delete;
        Builder(Builder &&) = delete;
        auto operator=(Builder &&) -> Builder & = delete;

        auto setVertexShader(const fs::path &path) -> Builder & override;
        auto setFragmentShader(const fs::path &path) -> Builder & override;
        auto setGeometryShader(const fs::path &path) -> Builder & override;
        auto setTessellationControlShader(const fs::path &path) -> Builder & override;
        auto setTessellationEvaluationShader(const fs::path &path) -> Builder & override;
        auto setComputeShader(const fs::path &path) -> Builder & override;

        auto setPrimitiveTopology(const PrimitiveTopology &topology) -> Builder & override;

        auto setPolygonMode(const PolygonMode &mode) -> Builder & override;
        auto setCullMode(const CullMode &mode) -> Builder & override;
        auto setFrontFace(const FrontFace &face) -> Builder & override;
        auto setLineWidth(f32 width) -> Builder & override;

        auto setDepthTest(bool enable) -> Builder & override;
        auto setDepthWrite(bool enable) -> Builder & override;
        auto setDepthCompareOp(const CompareOp &op) -> Builder & override;
        auto setStencilTest(bool enable) -> Builder & override;
        auto
        setStencilOp(const StencilOp &failOp, const StencilOp &passOp, const StencilOp &depthFailOp)
            -> Builder & override;

        auto setBlend(bool enable) -> Builder & override;
        auto setBlendFactor(
            const BlendFactor &srcColor,
            const BlendFactor &dstColor,
            const BlendFactor &srcAlpha,
            const BlendFactor &dstAlpha
        ) -> Builder & override;
        auto setBlendOp(const BlendOp &colorOp, const BlendOp &alphaOp) -> Builder & override;
        auto setColorWriteMask(const ColorComponentFlags &mask) -> Builder & override;

        auto addPushConstant(u32 size) -> Builder & override;

        auto setName(const std::string &name) -> Builder & override;

        auto build() -> std::expected<std::unique_ptr<Pipeline>, std::string> override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };

    void bind() override;

private:
    VulkanPipeline(VulkanDevice *device, VkPipeline pipeline, VkPipelineLayout layout);

    struct Factory
    {
        static auto create(VulkanDevice *device, VkPipeline pipeline, VkPipelineLayout layout)
            -> std::unique_ptr<VulkanPipeline>
        {
            return std::unique_ptr<VulkanPipeline>(new VulkanPipeline(device, pipeline, layout));
        }
    };

    friend struct Factory;

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vostok::graphics::vulkan