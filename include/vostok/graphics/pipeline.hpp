#pragma once

#include "vostok/core/type.hpp"

#include <expected>
#include <filesystem>
#include <optional>

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

struct PipelineCreateInfo
{
    std::optional<fs::path> vertexShader;
    std::optional<fs::path> fragmentShader;
    std::optional<fs::path> geometryShader;
    std::optional<fs::path> tessellationControlShader;
    std::optional<fs::path> tessellationEvaluationShader;
    std::optional<fs::path> computeShader;

    PrimitiveTopology primitiveTopology = PrimitiveTopology::TRIANGLE_LIST;
    PolygonMode polygonMode = PolygonMode::FILL;
    CullMode cullMode = CullMode::BACK;
    FrontFace frontFace = FrontFace::COUNTER_CLOCKWISE;
    f32 lineWidth = 1.0F;

    bool depthTest = true;
    bool depthWrite = true;
    CompareOp depthCompareOp = CompareOp::LESS;
    bool stencilTest = false;
    StencilOp stencilFailOp = StencilOp::KEEP;
    StencilOp stencilPassOp = StencilOp::KEEP;
    StencilOp stencilDepthFailOp = StencilOp::KEEP;

    bool blend = false;
    BlendFactor srcColorBlendFactor = BlendFactor::ONE;
    BlendFactor dstColorBlendFactor = BlendFactor::ZERO;
    BlendFactor srcAlphaBlendFactor = BlendFactor::ONE;
    BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO;
    BlendOp colorBlendOp = BlendOp::ADD;
    BlendOp alphaBlendOp = BlendOp::ADD;
    ColorComponentFlags colorWriteMask = ColorComponentFlags::ALL;

    size_t pushConstantSize = 0;

    std::string name;
};

class Pipeline
{
public:
    virtual ~Pipeline() = default;

    Pipeline(Pipeline &) = delete;
    auto operator=(const Pipeline &) -> Pipeline & = delete;
    Pipeline(Pipeline &&) = delete;
    auto operator=(Pipeline &&) -> Pipeline & = delete;

    virtual void bind() = 0;

    template <typename T>
        requires std::is_trivially_copyable_v<T> && (sizeof(T) <= 128)
    auto push(const T &data, u32 offset = 0) -> std::expected<void, std::string>
    {
        return pushRaw(std::bit_cast<const void *>(&data), sizeof(T), offset);
    }

protected:
    Pipeline() = default;

    virtual auto pushRaw(const void *data, size_t size, u32 offset = 0)
        -> std::expected<void, std::string> = 0;
};

} // namespace vostok::graphics