#pragma once

#include "vostok/math/matrix_ops.hpp"
#include "vostok/math/projections.hpp"
#include "vostok/math/quaternion_ops.hpp"
#include "vostok/math/types.hpp"
#include "vostok/math/vector_ops.hpp"

namespace vostok::math
{

using ::vostok::math::conjugate;
using ::vostok::math::convertQuaternionToMat4;
using ::vostok::math::cross;
using ::vostok::math::dot;
using ::vostok::math::inverse;
using ::vostok::math::length;
using ::vostok::math::normalize;
using ::vostok::math::ortho;
using ::vostok::math::perspective;
using ::vostok::math::quatLookAt;
using ::vostok::math::transpose;

using ::vostok::math::Mat4;

template <typename T>
    requires std::is_arithmetic_v<T>
[[nodiscard]] constexpr auto radians(T degrees) noexcept -> T
{
    return glm::radians(degrees);
}

template <typename T>
    requires std::is_arithmetic_v<T>
[[nodiscard]] constexpr auto degrees(T radians) noexcept -> T
{
    return glm::degrees(radians);
}

} // namespace vostok::math