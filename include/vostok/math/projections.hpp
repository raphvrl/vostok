#pragma once

#include <glm/gtc/matrix_transform.hpp>

namespace vostok::math
{

template <typename T>
    requires std::is_arithmetic_v<T>
[[nodiscard]] auto perspective(T fovy, T aspect, T near, T far) noexcept -> glm::mat<4, 4, T>
{
    return glm::perspective(fovy, aspect, near, far);
}

template <typename T>
    requires std::is_arithmetic_v<T>
[[nodiscard]] auto perspectiveInfinite(T fovy, T aspect, T near) noexcept -> glm::mat<4, 4, T>
{
    return glm::infinitePerspective(fovy, aspect, near);
}

template <typename T>
    requires std::is_arithmetic_v<T>
[[nodiscard]] auto ortho(T left, T right, T bottom, T top, T near, T far) noexcept
    -> glm::mat<4, 4, T>
{
    return glm::ortho(left, right, bottom, top, near, far);
}

template <typename T>
    requires std::is_arithmetic_v<T>
[[nodiscard]] auto frustum(T left, T right, T bottom, T top, T near, T far) noexcept
    -> glm::mat<4, 4, T>
{
    return glm::frustum(left, right, bottom, top, near, far);
}

} // namespace vostok::math