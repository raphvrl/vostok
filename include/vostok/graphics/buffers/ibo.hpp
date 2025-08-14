#pragma once

#include "vostok/core/type.hpp"
#include "vostok/graphics/buffers/buffer.hpp"

#include <memory>
#include <span>

namespace vostok::graphics
{

template <typename T>
class IBOImpl
{
public:
    IBOImpl(std::unique_ptr<Buffer> buffer, const std::span<const T> &data)
        : m_buffer(std::move(buffer)),
          m_data(data.begin(), data.end())
    {
        if (m_data.size() * sizeof(T) != m_buffer->getSize()) {
            throw std::runtime_error("Data size mismatch with buffer size");
        }
    }

    IBOImpl() = default;
    ~IBOImpl() = default;

    IBOImpl(const IBOImpl &other) noexcept = default;
    IBOImpl(IBOImpl &&other) noexcept = default;
    auto operator=(const IBOImpl &other) -> IBOImpl & = default;
    auto operator=(IBOImpl &&other) noexcept -> IBOImpl & = default;

    void bind() { m_buffer->bind(); }

    [[nodiscard]] auto getData() const noexcept -> const std::vector<T> &
    {
        return m_data;
    }

    [[nodiscard]] auto getData() noexcept -> std::vector<T> & { return m_data; }

    [[nodiscard]] auto getBuffer() const noexcept -> const Buffer *
    {
        return m_buffer.get();
    }

    [[nodiscard]] auto getBuffer() noexcept -> Buffer *
    {
        return m_buffer.get();
    }

    auto updateData(const std::span<const T> &newData)
        -> std::expected<void, std::string>
    {
        if (newData.size() != m_data.size()) {
            return std::unexpected("Data size mismatch");
        }

        m_data.assign(newData.begin(), newData.end());
        return m_buffer->updateArray(m_data);
    }

    [[nodiscard]] auto getIndexCount() const noexcept -> size_t
    {
        return m_data.size();
    }

    [[nodiscard]] auto getTotalSize() const noexcept -> size_t
    {
        return m_data.size() * sizeof(T);
    }

private:
    std::unique_ptr<Buffer> m_buffer;
    std::vector<T> m_data;
};

template <typename T>
class IBO
{
public:
    IBO() = default;
    ~IBO() = default;

    explicit IBO(std::unique_ptr<IBOImpl<T>> &&ibo)
        : m_ibo(std::move(ibo))
    {}

    IBO(IBO &&other) noexcept = default;
    auto operator=(IBO &&other) noexcept -> IBO & = default;

    IBO(const IBO &) noexcept = delete;
    auto operator=(const IBO &) noexcept -> IBO & = delete;

    auto operator->() -> IBOImpl<T> *
    {
        if (!m_ibo) {
            throw std::runtime_error("IBO is null");
        }
        return m_ibo.get();
    }

    auto operator->() const -> const IBOImpl<T> *
    {
        if (!m_ibo) {
            throw std::runtime_error("IBO is null");
        }
        return m_ibo.get();
    }

    auto operator*() -> IBOImpl<T> &
    {
        if (!m_ibo) {
            throw std::runtime_error("IBO is null");
        }
        return *m_ibo;
    }

    auto operator*() const -> const IBOImpl<T> &
    {
        if (!m_ibo) {
            throw std::runtime_error("IBO is null");
        }
        return *m_ibo;
    }

    auto get() -> IBOImpl<T> * { return m_ibo.get(); }
    auto get() const -> const IBOImpl<T> * { return m_ibo.get(); }

    explicit operator bool() const { return m_ibo != nullptr; }

    auto release() -> std::unique_ptr<IBOImpl<T>> { return std::move(m_ibo); }

private:
    std::unique_ptr<IBOImpl<T>> m_ibo;
};

} // namespace vostok::graphics