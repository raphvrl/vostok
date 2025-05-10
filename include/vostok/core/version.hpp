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

    [[nodiscard]] std::string toString() const
    {
        return std::format("{}.{}.{}", major, minor, patch);
    }

    [[nodiscard]] u32 toPackedInt() const
    {
        return (major << 22) | (minor << 12) | patch;
    }

    static Version fromPackedInt(u32 packed)
    {
        return Version{
            .major = (packed >> 22) & 0x3FF,
            .minor = (packed >> 12) & 0x3FF,
            .patch = packed & 0xFFF,
        };
    }

    bool operator==(const Version &other) const
    {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    bool operator!=(const Version &other) const
    {
        return !(*this == other);
    }

    bool operator<(const Version &other) const
    {
        if (major != other.major) {
            return major < other.major;
        }
        if (minor != other.minor) {
            return minor < other.minor;
        }
        return patch < other.patch;
    }

    bool operator>(const Version &other) const
    {
        return other < *this;
    }

    bool operator<=(const Version &other) const
    {
        return !(*this > other);
    }

    bool operator>=(const Version &other) const
    {
        return !(*this < other);
    }
};

} // namespace vostok::core