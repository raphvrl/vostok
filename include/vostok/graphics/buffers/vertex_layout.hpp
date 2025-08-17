#pragma once

#include "vostok/core/type.hpp"

#include <vector>

namespace vostok::graphics
{

enum class VertexFormat : u8
{
    FLOAT1,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    INT1,
    INT2,
    INT3,
    INT4,
    UINT1,
    UINT2,
    UINT3,
    UINT4,
};

enum class IndexType : u8
{
    UINT16,
    UINT32,
};

struct VertexAttribute
{
    u32 location;
    u32 offset;
    u32 size;
    VertexFormat format;

    VertexAttribute() = default;

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    VertexAttribute(u32 loc, u32 off, u32 sz, VertexFormat fmt)
        : location(loc),
          offset(off),
          size(sz),
          format(fmt)
    {}
};

struct VertexLayout
{
    std::vector<VertexAttribute> attributes;
    u32 stride = 0;

    VertexLayout() = default;

    VertexLayout(std::initializer_list<VertexAttribute> attrs)
        : attributes(attrs)
    {
        calculateStride();
    }

    explicit VertexLayout(const std::vector<VertexAttribute> &attrs)
        : attributes(attrs)
    {
        calculateStride();
    }

    explicit VertexLayout(std::vector<VertexAttribute> &&attrs)
        : attributes(std::move(attrs))
    {
        calculateStride();
    }

    void addAttribute(u32 location, u32 offset, u32 size, VertexFormat format);
    void calculateStride();
    [[nodiscard]] auto isValid() const -> bool;
};

auto getFormatSize(VertexFormat format) -> u32;
auto createVertexLayout(std::initializer_list<VertexFormat> formats)
    -> VertexLayout;

namespace formats
{
constexpr VertexFormat F32 = VertexFormat::FLOAT1;
constexpr VertexFormat VEC2 = VertexFormat::FLOAT2;
constexpr VertexFormat VEC3 = VertexFormat::FLOAT3;
constexpr VertexFormat VEC4 = VertexFormat::FLOAT4;
constexpr VertexFormat I32 = VertexFormat::INT1;
constexpr VertexFormat U32 = VertexFormat::UINT1;
} // namespace formats

template <typename T>
struct VertexLayoutTraits
{
    static constexpr auto getLayout() -> VertexLayout
    {
        return generateExplicitLayout<T>();
    }

private:
    template <typename U>
    static constexpr auto generateExplicitLayout() -> VertexLayout
    {
        if constexpr (std::is_same_v<U, struct Vertex>) {
            return createVertexLayout(
                { VertexFormat::FLOAT2, VertexFormat::FLOAT3 }
            );
        } else if constexpr (std::is_same_v<U, struct PositionNormal>) {
            return createVertexLayout(
                { VertexFormat::FLOAT3, VertexFormat::FLOAT3 }
            );
        } else if constexpr (std::is_same_v<U, struct PositionUV>) {
            return createVertexLayout(
                { VertexFormat::FLOAT3, VertexFormat::FLOAT2 }
            );
        } else {
            static_assert(
                false,
                "Type not supported. Use createVertexLayout() explicitly."
            );
        }
    }
};

template <typename T>
auto getVertexLayout() -> VertexLayout
{
    return VertexLayoutTraits<T>::getLayout();
}

template <typename T>
concept VertexType = requires() {
    { T::getLayout() } -> std::convertible_to<const VertexLayout &>;
};

} // namespace vostok::graphics