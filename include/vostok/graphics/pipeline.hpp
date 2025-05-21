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
    Pipeline &operator=(const Pipeline &) = delete;
    Pipeline(Pipeline &&) = delete;
    Pipeline &operator=(Pipeline &&) = delete;

    class Builder
    {
    public:
        Builder() = default;
        virtual ~Builder() = default;

        Builder(Builder &) = delete;
        Builder &operator=(const Builder &) = delete;
        Builder(Builder &&) = delete;
        Builder &operator=(Builder &&) = delete;

        virtual Builder &setVertexShader(const fs::path &path) = 0;
        virtual Builder &setFragmentShader(const fs::path &path) = 0;
        virtual Builder &setGeometryShader(const fs::path &path) = 0;
        virtual Builder &setTessellationControlShader(const fs::path &path) = 0;
        virtual Builder &setTessellationEvaluationShader(const fs::path &path) = 0;
        virtual Builder &setComputeShader(const fs::path &path) = 0;

        virtual Builder &setPrimitiveTopology(const PrimitiveTopology &topology) = 0;

        virtual Builder &setPolygonMode(const PolygonMode &mode) = 0;
        virtual Builder &setCullMode(const CullMode &mode) = 0;
        virtual Builder &setFrontFace(const FrontFace &face) = 0;
        virtual Builder &setLineWidth(f32 width) = 0;

        virtual Builder &setDepthTest(bool enable) = 0;
        virtual Builder &setDepthWrite(bool enable) = 0;
        virtual Builder &setDepthCompareOp(const CompareOp &op) = 0;
        virtual Builder &setStencilTest(bool enable) = 0;
        virtual Builder &setStencilOp(
            const StencilOp &failOp,
            const StencilOp &passOp,
            const StencilOp &depthFailOp
        ) = 0;

        virtual Builder &setBlend(bool enable) = 0;
        virtual Builder &setBlendFactor(
            const BlendFactor &srcColor,
            const BlendFactor &dstColor,
            const BlendFactor &srcAlpha,
            const BlendFactor &dstAlpha
        ) = 0;
        virtual Builder &setBlendOp(const BlendOp &colorOp, const BlendOp &alphaOp) = 0;
        virtual Builder &setColorWriteMask(const ColorComponentFlags &mask) = 0;

        virtual Builder &addPushConstant(u32 size);

        virtual Builder &setName(const std::string &name) = 0;

        virtual std::expected<std::unique_ptr<Pipeline>, std::string> build() = 0;
    };

    virtual void bind() = 0;

protected:
    Pipeline() = default;
};

} // namespace vostok::graphics