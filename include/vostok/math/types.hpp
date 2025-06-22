#pragma once

#include "vostok/core/type.hpp"

#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vostok::math
{

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;

using IVec2 = glm::ivec2;
using IVec3 = glm::ivec3;
using IVec4 = glm::ivec4;

using UVec2 = glm::uvec2;
using UVec3 = glm::uvec3;
using UVec4 = glm::uvec4;

using Mat2 = glm::mat2;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

using Quat = glm::quat;

constexpr f32 PI = glm::pi<f32>();
constexpr f32 TWO_PI = glm::two_pi<f32>();
constexpr f32 HALF_PI = glm::half_pi<f32>();
constexpr f32 EPSILON = glm::epsilon<f32>();

} // namespace vostok::math