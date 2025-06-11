#pragma once

#include "type.hpp"

#include <format>
#include <string>

namespace vostok::core
{

struct Version
{
    u32 major = 0;
    u32 minor = 0;
    u32 patch = 0;

    [[nodiscard]] auto toString() const -> std::string
    {
        return std::format("{}.{}.{}", major, minor, patch);
    }

    [[nodiscard]] auto toPackedInt() const -> u32 { return (major << 22) | (minor << 12) | patch; }

    static auto fromPackedInt(u32 packed) -> Version
    {
        return Version{
            .major = (packed >> 22) & 0x3FF,
            .minor = (packed >> 12) & 0x3FF,
            .patch = packed & 0xFFF,
        };
    }

    auto operator==(const Version &other) const -> bool
    {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    auto operator!=(const Version &other) const -> bool { return !(*this == other); }

    auto operator<(const Version &other) const -> bool
    {
        if (major != other.major) {
            return major < other.major;
        }
        if (minor != other.minor) {
            return minor < other.minor;
        }
        return patch < other.patch;
    }

    auto operator>(const Version &other) const -> bool { return other < *this; }

    auto operator<=(const Version &other) const -> bool { return !(*this > other); }

    auto operator>=(const Version &other) const -> bool { return !(*this < other); }
};

} // namespace vostok::core