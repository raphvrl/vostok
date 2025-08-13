#pragma once

#include "vostok/graphics/buffers/bindable_resource.hpp"

namespace vostok::graphics
{

template <typename T>
concept UBOType =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> &&
    sizeof(T) <= static_cast<u32>(64 * 1024); // 64KB max pour UBO

template <UBOType T>
class UBO : public BindableResource
{
public:
    using DataType = T;

    UBO() = default;
    ~UBO() = default;

    UBO(const UBO &other) noexcept = default;
    UBO(UBO &&other) noexcept = default;
    auto operator=(const UBO &other) -> UBO & = default;
    auto operator=(UBO &&other) noexcept -> UBO & = default;

    explicit UBO(const T &data)
        : BindableResource(),
          m_data(data)
    {
        markDirty();
    }

    explicit UBO(T &&data)
        : BindableResource(),
          m_data(std::move(data))
    {
        markDirty();
    }

    auto operator=(const T &data) -> UBO &
    {
        if (m_data != data) {
            m_data = data;
            markDirty();
        }
        return *this;
    }

    auto setUniform(const T &data) -> UBO & { return operator=(data); }

    [[nodiscard]] auto getData() const noexcept -> const T & { return m_data; }

    [[nodiscard]] auto getData() noexcept -> T & { return m_data; }

    [[nodiscard]] auto getRawData() const noexcept -> const void * override
    {
        return &m_data;
    }

    [[nodiscard]] auto getDataSize() const noexcept -> size_t override
    {
        return sizeof(T);
    }

private:
    T m_data{};
};

template <UBOType T>
class UBOPtr
{
public:
    UBOPtr() = default;
    ~UBOPtr() = default;

    explicit UBOPtr(std::unique_ptr<UBO<T>> &&ubo)
        : m_ubo(std::move(ubo))
    {}

    UBOPtr(UBOPtr &&other) noexcept = default;
    auto operator=(UBOPtr &&other) noexcept -> UBOPtr & = default;
    UBOPtr(const UBOPtr &) noexcept = delete;
    auto operator=(const UBOPtr &) noexcept -> UBOPtr & = delete;

    auto operator->() -> T *
    {
        if (!m_ubo) {
            throw std::runtime_error("UBOPtr is null");
        }
        m_ubo->markDirty();
        return &m_ubo->getData();
    }

    auto operator->() const -> const T *
    {
        if (!m_ubo) {
            throw std::runtime_error("UBOPtr is null");
        }
        return &m_ubo->getData();
    }

    auto operator*() -> T &
    {
        if (!m_ubo) {
            throw std::runtime_error("UBOPtr is null");
        }
        m_ubo->markDirty();
        return m_ubo->getData();
    }

    auto operator*() const -> const T &
    {
        if (!m_ubo) {
            throw std::runtime_error("UBOPtr is null");
        }
        return m_ubo->getData();
    }

    auto get() -> UBO<T> * { return m_ubo.get(); }
    auto get() const -> const UBO<T> * { return m_ubo.get(); }

    explicit operator bool() const { return m_ubo != nullptr; }

    auto release() -> std::unique_ptr<UBO<T>> { return std::move(m_ubo); }

private:
    std::unique_ptr<UBO<T>> m_ubo;
};

} // namespace vostok::graphics