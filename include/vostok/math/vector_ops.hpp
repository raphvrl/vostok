#pragma once

#include "vostok/math/concepts.hpp"

#include <glm/glm.hpp>

namespace vostok::math
{

template <VectorType T>
[[nodiscard]] constexpr auto normalize(const T &v) noexcept -> T
{
    return glm::normalize(v);
}

template <VectorType T>
[[nodiscard]] constexpr auto length(const T &v) noexcept ->
    typename T::value_type
{
    return glm::length(v);
}

template <VectorType T>
[[nodiscard]] constexpr auto dot(const T &a, const T &b) noexcept ->
    typename T::value_type
{
    return glm::dot(a, b);
}

template <VectorType T>
    requires(T::length() == 3)
[[nodiscard]] constexpr auto cross(const T &a, const T &b) noexcept -> T
{
    return glm::cross(a, b);
}

template <VectorType T>
[[nodiscard]] constexpr auto distance(const T &a, const T &b) noexcept ->
    typename T::value_type
{
    return glm::distance(a, b);
}

template <VectorType T>
[[nodiscard]] constexpr auto
reflect(const T &incident, const T &normal) noexcept -> T
{
    return glm::reflect(incident, normal);
}

template <VectorType T>
[[nodiscard]] constexpr auto
refract(const T &incident, const T &normal, typename T::value_type eta) noexcept
    -> T
{
    return glm::refract(incident, normal, eta);
}

} // namespace vostok::math