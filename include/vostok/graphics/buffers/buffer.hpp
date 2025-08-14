#pragma once

#include "vostok/core/type.hpp"

#include <algorithm>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace vostok::graphics
{

enum class BufferUsage : u8
{
    VERTEX = 1 << 0,
    INDEX = 1 << 1,
    UNIFORM = 1 << 2,
    STORAGE = 1 << 3,
    TRANSFER_SRC = 1 << 4,
    TRANSFER_DST = 1 << 5,
};

inline auto operator|(BufferUsage lhs, BufferUsage rhs) -> BufferUsage
{
    return static_cast<BufferUsage>(
        static_cast<u32>(lhs) | static_cast<u32>(rhs)
    );
}

inline auto operator&(BufferUsage lhs, BufferUsage rhs) -> BufferUsage
{
    return static_cast<BufferUsage>(
        static_cast<u32>(lhs) & static_cast<u32>(rhs)
    );
}

inline auto operator~(BufferUsage usage) -> BufferUsage
{
    return static_cast<BufferUsage>(~static_cast<u32>(usage));
}

enum class BufferMemory : u8
{
    GPU_ONLY,
    CPU_TO_GPU,
    GPU_TO_CPU,
};

struct BufferCreateInfo
{
    BufferUsage usage;
    BufferMemory memory;
    size_t size;
    std::span<const std::byte> data;
    std::string debugName;
};

class Buffer
{
public:
    virtual ~Buffer() = default;

    Buffer(const Buffer &) = delete;
    auto operator=(const Buffer &) -> Buffer & = delete;
    Buffer(Buffer &&) = delete;
    auto operator=(Buffer &&) -> Buffer & = delete;

    virtual void bind() = 0;

    virtual auto map() -> std::expected<std::span<std::byte>, std::string> = 0;
    virtual void unmap() = 0;

    virtual auto update(std::span<const std::byte> data, size_t offset = 0)
        -> std::expected<void, std::string> = 0;

    virtual auto copyFrom(
        const Buffer &source,
        size_t srcOffset = 0,
        size_t dstOffset = 0,
        size_t size = 0
    ) -> std::expected<void, std::string> = 0;

    [[nodiscard]] virtual auto getSize() const -> size_t = 0;
    [[nodiscard]] virtual auto getUsage() const -> BufferUsage = 0;
    [[nodiscard]] virtual auto getMemory() const -> BufferMemory = 0;

    [[nodiscard]] auto operator==(const Buffer &other) const -> bool
    {
        return getSize() == other.getSize() && getUsage() == other.getUsage() &&
               getMemory() == other.getMemory();
    }

    [[nodiscard]] auto operator!=(const Buffer &other) const -> bool
    {
        return !(*this == other);
    }

    [[nodiscard]] auto operator<(const Buffer &other) const -> bool
    {
        return getSize() < other.getSize();
    }

    [[nodiscard]] auto operator<=(const Buffer &other) const -> bool
    {
        return getSize() <= other.getSize();
    }

    [[nodiscard]] auto operator>(const Buffer &other) const -> bool
    {
        return getSize() > other.getSize();
    }

    [[nodiscard]] auto operator>=(const Buffer &other) const -> bool
    {
        return getSize() >= other.getSize();
    }

    template <typename T>
    auto updateData(const T &data, size_t offset = 0)
        -> std::expected<void, std::string>
    {
        return update(
            std::span<const std::byte>(
                std::bit_cast<const std::byte *>(&data),
                sizeof(T)
            ),
            offset
        );
    }

    template <typename T>
    auto updateArray(const std::vector<T> &data, size_t offset = 0)
        -> std::expected<void, std::string>
    {
        return update(
            std::span<const std::byte>(
                std::bit_cast<const std::byte *>(data.data()),
                data.size() * sizeof(T)
            ),
            offset
        );
    }

    auto fill(std::byte value, size_t offset = 0, size_t size = 0)
        -> std::expected<void, std::string>
    {
        if (size == 0) {
            size = getSize() - offset;
        }

        if (offset + size > getSize()) {
            return std::unexpected("Fill operation exceeds buffer size");
        }

        auto mapped = map();
        if (!mapped) {
            return std::unexpected(
                "Failed to map buffer for fill operation: " + mapped.error()
            );
        }

        std::fill(
            mapped->begin() + static_cast<std::ptrdiff_t>(offset),
            mapped->begin() + static_cast<std::ptrdiff_t>(offset) +
                static_cast<std::ptrdiff_t>(size),
            value
        );
        unmap();
        return {};
    }

    [[nodiscard]] auto isEmpty() const -> bool { return getSize() == 0; }

    [[nodiscard]] auto getAvailableSize(size_t offset = 0) const -> size_t
    {
        return offset < getSize() ? getSize() - offset : 0;
    }

    [[nodiscard]] auto isValidOffset(size_t offset) const -> bool
    {
        return offset < getSize();
    }

    [[nodiscard]] auto isValidRange(size_t offset, size_t size) const -> bool
    {
        return offset < getSize() && size <= getSize() - offset;
    }

    [[nodiscard]] virtual auto getOffset() const -> size_t = 0;
    virtual auto setOffset(size_t offset) -> void = 0;

protected:
    Buffer() = default;
};

} // namespace vostok::graphics