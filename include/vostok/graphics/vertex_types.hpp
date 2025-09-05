#pragma once

#include "vostok/graphics/buffers/vertex_layout.hpp"
#include "vostok/math/types.hpp"

namespace vostok::graphics
{

using Position = math::Vec3;
using Normal = math::Vec3;
using UV = math::Vec2;
using Tangent = math::Vec4;
using Color = math::Vec4;

struct PositionOnly
{
    Position position;

    static constexpr auto getLayout() -> const VertexLayout &
    {
        static const auto LAYOUT = createVertexLayout({ formats::VEC3 });
        return LAYOUT;
    }
};

struct PositionUV
{
    Position position;
    UV uv;

    static constexpr auto getLayout() -> const VertexLayout &
    {
        static const auto LAYOUT =
            createVertexLayout({ formats::VEC3, formats::VEC2 });
        return LAYOUT;
    }
};

struct PositionNormal
{
    Position position;
    Normal normal;

    static constexpr auto getLayout() -> const VertexLayout &
    {
        static const auto LAYOUT =
            createVertexLayout({ formats::VEC3, formats::VEC3 });
        return LAYOUT;
    }
};

struct PositionNormalUV
{
    Position position;
    Normal normal;
    UV uv;

    static constexpr auto getLayout() -> const VertexLayout &
    {
        static const auto LAYOUT =
            createVertexLayout({ formats::VEC3, formats::VEC3, formats::VEC2 });
        return LAYOUT;
    }
};

struct PBRVertex
{
    Position position;
    Normal normal;
    Tangent tangent;
    UV uv;
    Color color;

    static constexpr auto getLayout() -> const VertexLayout &
    {
        static const auto LAYOUT = createVertexLayout(
            { formats::VEC3,
              formats::VEC3,
              formats::VEC4,
              formats::VEC2,
              formats::VEC4 }
        );
        return LAYOUT;
    }
};

} // namespace vostok::graphics