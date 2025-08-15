#pragma once

#include "vostok/core/type.hpp"

#include <expected>
#include <string>
#include <variant>

namespace vostok::graphics
{

enum class ImageUsage : u8
{
    TRANSFER_SRC = 1 << 0,
    TRANSFER_DST = 1 << 1,
    SAMPLED = 1 << 2,
    STORAGE = 1 << 3,
    COLOR_ATTACHMENT = 1 << 4,
    DEPTH_STENCIL_ATTACHMENT = 1 << 5,
    TRANSIENT_ATTACHMENT = 1 << 6,
    INPUT_ATTACHMENT = 1 << 7,
};

inline auto operator|(ImageUsage lhs, ImageUsage rhs) -> ImageUsage
{
    return static_cast<ImageUsage>(
        static_cast<u32>(lhs) | static_cast<u32>(rhs)
    );
}

inline auto operator&(ImageUsage lhs, ImageUsage rhs) -> ImageUsage
{
    return static_cast<ImageUsage>(
        static_cast<u32>(lhs) & static_cast<u32>(rhs)
    );
}

enum class ImageFormat : u8
{
    UNDEFINED,
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    B8G8R8A8_UNORM,
    B8G8R8A8_SRGB,
    R32G32B32A32_SFLOAT,
    D32_SFLOAT,
    D24_UNORM_S8_UINT,
};

enum class ImageLayout : u8
{
    UNDEFINED,
    GENERAL,
    COLOR_ATTACHMENT_OPTIMAL,
    DEPTH_ATTACHMENT_OPTIMAL,
    DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    SHADER_READ_ONLY_OPTIMAL,
    TRANSFER_SRC_OPTIMAL,
    TRANSFER_DST_OPTIMAL,
    PRESENT_SRC_KHR,
};

enum class SampleCount : u8
{
    COUNT_1 = 1,
    COUNT_2 = 2,
    COUNT_4 = 4,
    COUNT_8 = 8,
    COUNT_16 = 16,
};

enum class ImageAspectFlags : u8
{
    COLOR = 1 << 0,
    DEPTH = 1 << 1,
    STENCIL = 1 << 2,
    DEPTH_STENCIL = DEPTH | STENCIL,
    METADATA = 1 << 3,
};

inline auto operator|(ImageAspectFlags lhs, ImageAspectFlags rhs)
    -> ImageAspectFlags
{
    return static_cast<ImageAspectFlags>(
        static_cast<u32>(lhs) | static_cast<u32>(rhs)
    );
}

inline auto operator&(ImageAspectFlags lhs, ImageAspectFlags rhs)
    -> ImageAspectFlags
{
    return static_cast<ImageAspectFlags>(
        static_cast<u32>(lhs) & static_cast<u32>(rhs)
    );
}

struct ClearValue
{
    struct Color
    {
        f32 r, g, b, a;
    };

    struct DepthStencil
    {
        f32 depth;
        u32 stencil;
    };

    std::variant<Color, DepthStencil> value;

    ClearValue()
        : value(Color{ .r = 0.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F })
    {}

    explicit ClearValue(f32 r, f32 g, f32 b, f32 a = 1.0F)
        : value(Color{ .r = r, .g = g, .b = b, .a = a })
    {}

    explicit ClearValue(f32 depth, u32 stencil = 0)
        : value(DepthStencil{ .depth = depth, .stencil = stencil })
    {}

    [[nodiscard]] auto isColor() const -> bool
    {
        return std::holds_alternative<Color>(value);
    }
    [[nodiscard]] auto isDepthStencil() const -> bool
    {
        return std::holds_alternative<DepthStencil>(value);
    }

    [[nodiscard]] auto getColor() const -> const Color &
    {
        return std::get<Color>(value);
    }
    [[nodiscard]] auto getDepthStencil() const -> const DepthStencil &
    {
        return std::get<DepthStencil>(value);
    }
};

struct ImageCreateInfo
{
    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;
    u32 mipLevels = 1;
    u32 arrayLayers = 1;
    ImageFormat format = ImageFormat::UNDEFINED;
    ImageUsage usage = ImageUsage::COLOR_ATTACHMENT;
    SampleCount samples = SampleCount::COUNT_1;
    std::string debugName;
};

class Image
{
public:
    virtual ~Image() = default;

    Image(const Image &) = delete;
    auto operator=(const Image &) -> Image & = delete;
    Image(Image &&) = delete;
    auto operator=(Image &&) -> Image & = delete;

    virtual auto transitionLayout(
        ImageLayout oldLayout,
        ImageLayout newLayout,
        ImageAspectFlags aspectMask = ImageAspectFlags::COLOR
    ) -> std::expected<void, std::string> = 0;

    virtual auto clear(
        const ClearValue &clearValue,
        ImageAspectFlags aspectMask = ImageAspectFlags::COLOR
    ) -> std::expected<void, std::string> = 0;

    [[nodiscard]] virtual auto getWidth() const -> u32 = 0;
    [[nodiscard]] virtual auto getHeight() const -> u32 = 0;
    [[nodiscard]] virtual auto getDepth() const -> u32 = 0;
    [[nodiscard]] virtual auto getFormat() const -> ImageFormat = 0;
    [[nodiscard]] virtual auto getUsage() const -> ImageUsage = 0;
    [[nodiscard]] virtual auto getMipLevels() const -> u32 = 0;
    [[nodiscard]] virtual auto getArrayLayers() const -> u32 = 0;
    [[nodiscard]] virtual auto isDepthStencil() const -> bool = 0;
    [[nodiscard]] virtual auto isColor() const -> bool = 0;
    [[nodiscard]] virtual auto getAspectMask() const -> ImageAspectFlags = 0;

protected:
    Image() = default;
};

} // namespace vostok::graphics