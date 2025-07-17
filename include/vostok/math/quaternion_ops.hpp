#pragma once

#include "vostok/math/concepts.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vostok::math
{

template <QuaternionType T>
[[nodiscard]] constexpr auto conjugate(const T &q) noexcept -> T
{
    return glm::conjugate(q);
}

template <QuaternionType T>
[[nodiscard]] constexpr auto inverse(const T &q) noexcept -> T
{
    return glm::inverse(q);
}

template <QuaternionType T>
[[nodiscard]] constexpr auto normalize(const T &q) noexcept -> T
{
    return glm::normalize(q);
}

template <VectorType Vec>
    requires(Vec::length() == 3)
[[nodiscard]] auto quatLookAt(const Vec &direction, const Vec &up) noexcept
    -> glm::qua<typename Vec::value_type>
{
    return glm::quatLookAt(direction, up);
}

template <QuaternionType Quat>
[[nodiscard]] auto convertQuaternionToMat4(const Quat &q) noexcept
    -> glm::mat<4, 4, typename Quat::value_type>
{
    return glm::mat4_cast(q);
}

template <MatrixType Mat>
[[nodiscard]] auto convertMat4ToQuaternion(const Mat &m) noexcept
    -> glm::qua<typename Mat::value_type>
{
    return glm::quat_cast(m);
}

template <typename T, VectorType Vec>
    requires(std::is_arithmetic_v<T> && Vec::length() == 3)
[[nodiscard]] auto angleAxis(T angle, const Vec &axis) noexcept -> glm::qua<T>
{
    return glm::angleAxis(angle, axis);
}

template <QuaternionType T>
[[nodiscard]] constexpr auto angle(const T &q) noexcept ->
    typename T::value_type
{
    return glm::angle(q);
}

} // namespace vostok::math