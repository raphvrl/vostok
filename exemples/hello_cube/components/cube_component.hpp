#pragma once

#include "vostok/graphics/mesh.hpp"
#include "vostok/graphics/vertex_types.hpp"

#include <utility>

using namespace vostok;
using namespace vostok::graphics;

struct CubeComponent
{
    MeshHandle<PositionNormalUV, u32> *m_mesh = nullptr;
    std::filesystem::path m_texturePath;

    CubeComponent() = default;

    CubeComponent(
        MeshHandle<PositionNormalUV, u32> *mesh,
        std::filesystem::path texturePath
    )
        : m_mesh(mesh),
          m_texturePath(std::move(texturePath))
    {}
};