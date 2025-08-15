#pragma once

#include "vostok/graphics/buffers/bindable_resource.hpp"
#include "vostok/graphics/buffers/image.hpp"

#include <memory>
#include <span>

namespace vostok::graphics
{

enum class Filter : u8
{
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR
};

enum class AddressMode : u8
{
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER
};

struct TextureCreateInfo
{
    std::span<const std::byte> imageData;
    u32 width;
    u32 height;
    ImageFormat format;
    ImageUsage usage = ImageUsage::SAMPLED | ImageUsage::TRANSFER_DST;

    Filter magFilter = Filter::LINEAR;
    Filter minFilter = Filter::LINEAR;
    AddressMode addressModeU = AddressMode::REPEAT;
    AddressMode addressModeV = AddressMode::REPEAT;

    bool generateMipmaps = true;
    std::string debugName;
};

class TextureImpl : public BindableResource
{
public:
    explicit TextureImpl(const TextureCreateInfo &createInfo)
        : m_imageData(createInfo.imageData),
          m_width(createInfo.width),
          m_height(createInfo.height),
          m_format(createInfo.format),
          m_usage(createInfo.usage),
          m_magFilter(createInfo.magFilter)
    {}

    ~TextureImpl() override = default;

    TextureImpl(const TextureImpl &) = delete;
    auto operator=(const TextureImpl &) -> TextureImpl & = delete;

    TextureImpl(TextureImpl &&other) noexcept
        : m_imageData(other.m_imageData),
          m_width(other.m_width),
          m_height(other.m_height),
          m_format(other.m_format),
          m_usage(other.m_usage),
          m_magFilter(other.m_magFilter)
    {}

    auto operator=(TextureImpl &&other) noexcept -> TextureImpl &
    {
        m_imageData = other.m_imageData;
        m_width = other.m_width;
        m_height = other.m_height;
        m_format = other.m_format;
        m_usage = other.m_usage;
        m_magFilter = other.m_magFilter;
        return *this;
    }

    [[nodiscard]] auto getDataSize() const noexcept -> size_t override
    {
        return m_imageData.size();
    }

    [[nodiscard]] auto getRawData() const -> const void * override
    {
        return m_imageData.data();
    }

    [[nodiscard]] auto getWidth() const -> u32 { return m_width; }
    [[nodiscard]] auto getHeight() const -> u32 { return m_height; }
    [[nodiscard]] auto getFormat() const -> ImageFormat { return m_format; }
    [[nodiscard]] auto getUsage() const -> ImageUsage { return m_usage; }
    [[nodiscard]] auto getImageData() const
        -> const std::span<const std::byte> &
    {
        return m_imageData;
    }

private:
    std::span<const std::byte> m_imageData;
    u32 m_width;
    u32 m_height;
    ImageFormat m_format;
    ImageUsage m_usage;
    Filter m_magFilter;
};

class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    explicit Texture(std::unique_ptr<TextureImpl> &&texture)
        : m_texture(std::move(texture))
    {}

    Texture(Texture &&other) noexcept = default;
    auto operator=(Texture &&other) noexcept -> Texture & = default;
    Texture(const Texture &) = delete;
    auto operator=(const Texture &) -> Texture & = delete;

    auto operator->() -> TextureImpl * { return m_texture.get(); }
    auto operator->() const -> const TextureImpl * { return m_texture.get(); }

    auto get() -> TextureImpl * { return m_texture.get(); }
    [[nodiscard]] auto get() const -> const TextureImpl *
    {
        return m_texture.get();
    }

    explicit operator bool() const { return m_texture != nullptr; }

    auto release() -> std::unique_ptr<TextureImpl>
    {
        return std::move(m_texture);
    }

private:
    std::unique_ptr<TextureImpl> m_texture;
};

}; // namespace vostok::graphics