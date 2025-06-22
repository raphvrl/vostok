#pragma once

#include <concepts>
#include <type_traits>

namespace vostok::math
{

template <typename T>
concept ArithmeticType = std::is_arithmetic_v<T>;

template <typename T>
concept VectorType = requires {
    typename T::value_type;
    { T::length() } -> std::convertible_to<int>;
    requires ArithmeticType<typename T::value_type>;
};

template <typename T>
concept Vec2Type = VectorType<T> && (T::length() == 2);

template <typename T>
concept Vec3Type = VectorType<T> && (T::length() == 3);

template <typename T>
concept Vec4Type = VectorType<T> && (T::length() == 4);

template <typename T>
concept QuaternionType = requires {
    typename T::value_type;
    { std::declval<T>().w } -> std::convertible_to<typename T::value_type>;
    { std::declval<T>().x } -> std::convertible_to<typename T::value_type>;
    { std::declval<T>().y } -> std::convertible_to<typename T::value_type>;
    { std::declval<T>().z } -> std::convertible_to<typename T::value_type>;
    requires ArithmeticType<typename T::value_type>;
};

template <typename T>
concept MatrixType = requires {
    typename T::value_type;
    typename T::col_type;
    typename T::row_type;
    requires ArithmeticType<typename T::value_type>;
};

} // namespace vostok::math