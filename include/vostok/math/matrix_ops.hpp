#pragma once

#include "vostok/math/concepts.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vostok::math
{

template <MatrixType T>
[[nodiscard]] constexpr auto inverse(const T &m) noexcept -> T
{
    return glm::inverse(m);
}

template <MatrixType T>
[[nodiscard]] constexpr auto transpose(const T &m) noexcept -> T
{
    return glm::transpose(m);
}

template <MatrixType T>
[[nodiscard]] constexpr auto determinant(const T &m) noexcept -> typename T::value_type
{
    return glm::determinant(m);
}

template <MatrixType T>
[[nodiscard]] constexpr auto identity() noexcept -> T
{
    return T(1.0F);
}

template <VectorType Vec>
    requires(Vec::length() == 3)
[[nodiscard]] auto lookAt(const Vec &eye, const Vec &center, const Vec &up) noexcept
    -> glm::mat<4, 4, typename Vec::value_type>
{
    return glm::lookAt(eye, center, up);
}

} // namespace vostok::math