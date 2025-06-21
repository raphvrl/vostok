#pragma once

#include "vostok/core/type.hpp"

#include <expected>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

namespace vostok::graphics
{

enum class PrimitiveTopology : u8
{
    POINT_LIST,
    LINE_LIST,
    LINE_STRIP,
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
    LINE_LIST_WITH_ADJACENCY,
    LINE_STRIP_WITH_ADJACENCY,
    TRIANGLE_LIST_WITH_ADJACENCY,
    TRIANGLE_STRIP_WITH_ADJACENCY,
    PATCH_LIST
};

enum class PolygonMode : u8
{
    FILL,
    LINE,
    POINT,
    FILL_LINE,
    SOLID
};

enum class CullMode : u8
{
    NONE,
    FRONT,
    BACK,
    FRONT_AND_BACK
};

enum class FrontFace : u8
{
    COUNTER_CLOCKWISE,
    CLOCKWISE
};

enum class CompareOp : u8
{
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS
};

enum class StencilOp : u8
{
    KEEP,
    ZERO,
    REPLACE,
    INCREMENT_AND_CLAMP,
    DECREMENT_AND_CLAMP,
    INVERT,
    INCREMENT_AND_WRAP,
    DECREMENT_AND_WRAP
};

enum class BlendFactor : u8
{
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA
};

enum class BlendOp : u8
{
    ADD,
    SUBTRACT,
    REVERSE_SUBTRACT,
    MIN,
    MAX
};

enum class ColorComponentFlags : u8
{
    RED = 0x1,
    GREEN = 0x2,
    BLUE = 0x4,
    ALPHA = 0x8,
    ALL = RED | GREEN | BLUE | ALPHA
};

class Pipeline
{
public:
    virtual ~Pipeline() = default;

    Pipeline(Pipeline &) = delete;
    auto operator=(const Pipeline &) -> Pipeline & = delete;
    Pipeline(Pipeline &&) = delete;
    auto operator=(Pipeline &&) -> Pipeline & = delete;

    class Builder
    {
    public:
        Builder() = default;
        virtual ~Builder() = default;

        Builder(Builder &) = delete;
        auto operator=(const Builder &) -> Builder & = delete;
        Builder(Builder &&) = delete;
        auto operator=(Builder &&) -> Builder & = delete;

        virtual auto setVertexShader(const fs::path &path) -> Builder & = 0;
        virtual auto setFragmentShader(const fs::path &path) -> Builder & = 0;
        virtual auto setGeometryShader(const fs::path &path) -> Builder & = 0;
        virtual auto setTessellationControlShader(const fs::path &path) -> Builder & = 0;
        virtual auto setTessellationEvaluationShader(const fs::path &path) -> Builder & = 0;
        virtual auto setComputeShader(const fs::path &path) -> Builder & = 0;

        virtual auto setPrimitiveTopology(const PrimitiveTopology &topology) -> Builder & = 0;

        virtual auto setPolygonMode(const PolygonMode &mode) -> Builder & = 0;
        virtual auto setCullMode(const CullMode &mode) -> Builder & = 0;
        virtual auto setFrontFace(const FrontFace &face) -> Builder & = 0;
        virtual auto setLineWidth(f32 width) -> Builder & = 0;

        virtual auto setDepthTest(bool enable) -> Builder & = 0;
        virtual auto setDepthWrite(bool enable) -> Builder & = 0;
        virtual auto setDepthCompareOp(const CompareOp &op) -> Builder & = 0;
        virtual auto setStencilTest(bool enable) -> Builder & = 0;
        virtual auto
        setStencilOp(const StencilOp &failOp, const StencilOp &passOp, const StencilOp &depthFailOp)
            -> Builder & = 0;

        virtual auto setBlend(bool enable) -> Builder & = 0;
        virtual auto setBlendFactor(
            const BlendFactor &srcColor,
            const BlendFactor &dstColor,
            const BlendFactor &srcAlpha,
            const BlendFactor &dstAlpha
        ) -> Builder & = 0;
        virtual auto setBlendOp(const BlendOp &colorOp, const BlendOp &alphaOp) -> Builder & = 0;
        virtual auto setColorWriteMask(const ColorComponentFlags &mask) -> Builder & = 0;

        virtual auto addPushConstant(u32 size) -> Builder & = 0;

        virtual auto setName(const std::string &name) -> Builder & = 0;

        virtual auto build() -> std::expected<std::unique_ptr<Pipeline>, std::string> = 0;
    };

    virtual void bind() = 0;

protected:
    Pipeline() = default;
};

} // namespace vostok::graphics