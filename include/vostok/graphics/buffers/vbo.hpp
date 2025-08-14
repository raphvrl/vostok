#pragma once

#include "vostok/graphics/buffers/buffer.hpp"
#include "vostok/graphics/buffers/vertex_layout.hpp"

#include <memory>
#include <utility>

namespace vostok::graphics
{

template <typename T>
class VBOImpl
{
public:
    VBOImpl(
        std::unique_ptr<Buffer> buffer,
        const std::span<const T> &data,
        VertexLayout layout
    )
        : m_buffer(std::move(buffer)),
          m_data(data.begin(), data.end()),
          m_layout(std::move(layout))
    {
        if (!m_layout.isValid()) {
            throw std::runtime_error("Invalid vertex layout for VBO");
        }

        if (m_data.size() * m_layout.stride != m_buffer->getSize()) {
            throw std::runtime_error("Data size mismatch with buffer size");
        }
    }

    VBOImpl() = default;
    ~VBOImpl() = default;

    VBOImpl(const VBOImpl &other) noexcept = default;
    VBOImpl(VBOImpl &&other) noexcept = default;
    auto operator=(const VBOImpl &other) -> VBOImpl & = default;
    auto operator=(VBOImpl &&other) noexcept -> VBOImpl & = default;

    void bind() { m_buffer->bind(); }

    [[nodiscard]] auto getData() const noexcept -> const std::vector<T> &
    {
        return m_data;
    }

    [[nodiscard]] auto getData() noexcept -> std::vector<T> & { return m_data; }

    [[nodiscard]] auto getLayout() const noexcept -> const VertexLayout &
    {
        return m_layout;
    }

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

    auto appendData(const std::span<const T> &additionalData)
        -> std::expected<void, std::string>
    {
        m_data
            .insert(m_data.end(), additionalData.begin(), additionalData.end());

        if (m_data.size() * m_layout.stride > m_buffer->getSize()) {
            return std::unexpected("Buffer resize not implemented yet");
        }

        return m_buffer->updateArray(m_data);
    }

    [[nodiscard]] auto getVertexCount() const noexcept -> size_t
    {
        return m_data.size();
    }

    [[nodiscard]] auto getTotalSize() const noexcept -> size_t
    {
        return m_data.size() * m_layout.stride;
    }

private:
    std::unique_ptr<Buffer> m_buffer;
    std::vector<T> m_data;
    VertexLayout m_layout;
};

template <typename T>
class VBO
{
public:
    VBO() = default;
    ~VBO() = default;

    explicit VBO(std::unique_ptr<VBOImpl<T>> &&vbo)
        : m_vbo(std::move(vbo))
    {}

    VBO(VBO &&other) noexcept = default;
    auto operator=(VBO &&other) noexcept -> VBO & = default;

    VBO(const VBO &) noexcept = delete;
    auto operator=(const VBO &) noexcept -> VBO & = delete;

    auto operator->() -> VBOImpl<T> *
    {
        if (!m_vbo) {
            throw std::runtime_error("VBO is null");
        }
        return m_vbo.get();
    }

    auto operator->() const -> const VBOImpl<T> *
    {
        if (!m_vbo) {
            throw std::runtime_error("VBO is null");
        }
        return m_vbo.get();
    }

    auto operator*() -> VBOImpl<T> &
    {
        if (!m_vbo) {
            throw std::runtime_error("VBO is null");
        }
        return *m_vbo;
    }

    auto operator*() const -> const VBOImpl<T> &
    {
        if (!m_vbo) {
            throw std::runtime_error("VBO is null");
        }
        return *m_vbo;
    }

    auto get() -> VBOImpl<T> * { return m_vbo.get(); }
    auto get() const -> const VBOImpl<T> * { return m_vbo.get(); }

    explicit operator bool() const { return m_vbo != nullptr; }

    auto release() -> std::unique_ptr<VBOImpl<T>> { return std::move(m_vbo); }

private:
    std::unique_ptr<VBOImpl<T>> m_vbo;
};

} // namespace vostok::graphics