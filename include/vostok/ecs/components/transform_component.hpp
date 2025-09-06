#pragma once

#include "vostok/math/types.hpp"

namespace vostok::ecs
{

struct TransformComponent
{
    math::Vec3 position{ 0.0F, 0.0F, 0.0F };
    math::Quat rotation{ 1.0F, 0.0F, 0.0F, 0.0F };
    math::Vec3 scale{ 1.0F, 1.0F, 1.0F };

    TransformComponent() = default;

    TransformComponent(
        const math::Vec3 &position,
        const math::Quat &rotation,
        const math::Vec3 &scale
    )
        : position(position),
          rotation(rotation),
          scale(scale)
    {}
};

} // namespace vostok::ecs