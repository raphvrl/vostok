#pragma once

#include "vostok/core/type.hpp"

namespace vostok::colors
{

struct Color
{
    constexpr Color(u8 r, u8 g, u8 b) noexcept
        : red(r),
          green(g),
          blue(b)
    {}

    constexpr operator u32() const noexcept
    {
        return (static_cast<u32>(red) << 16) | (static_cast<u32>(green) << 8) |
               static_cast<u32>(blue);
    }

    u8 red;
    u8 green;
    u8 blue;
};

inline constexpr Color RED{ 255, 0, 0 };
inline constexpr Color GREEN{ 0, 255, 0 };
inline constexpr Color BLUE{ 0, 0, 255 };

inline constexpr Color YELLOW{ 255, 255, 0 };
inline constexpr Color MAGENTA{ 255, 0, 255 };
inline constexpr Color CYAN{ 0, 255, 255 };
inline constexpr Color ORANGE{ 255, 165, 0 };

inline constexpr Color PURPLE{ 128, 0, 128 };
inline constexpr Color LIGHT_BLUE{ 0, 128, 255 };
inline constexpr Color PINK{ 255, 192, 203 };
inline constexpr Color BROWN{ 139, 69, 19 };
inline constexpr Color GRAY{ 128, 128, 128 };
inline constexpr Color WHITE{ 255, 255, 255 };
inline constexpr Color BLACK{ 0, 0, 0 };

} // namespace vostok::colors