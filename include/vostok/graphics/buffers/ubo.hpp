#pragma once

#include "vostok/graphics/buffers/bindable_resource.hpp"

#include <memory>

namespace vostok::graphics
{
template <BindableType T>
class UBO : public BindableResource<T>
{
public:
    using DataType = T;

    UBO() = default;
    ~UBO() = default;

    explicit UBO(const T &data)
        : m_data(data)
    {
        this->markDirty();
    }

    explicit UBO(T &&data)
        : m_data(std::move(data))
    {
        this->markDirty();
    }

    UBO(const UBO &other)
        : BindableResource<T>(other),
          m_data(other.m_data)
    {
        this->markDirty();
    }

    UBO(UBO &&other) noexcept
        : BindableResource<T>(std::move(other)),
          m_data(std::move(other.m_data))
    {
        this->markDirty();
        other.markClean();
    }

    auto operator=(const UBO &other) -> UBO &
    {
        if (this != &other) {
            BindableResource<T>::operator=(other);
            m_data = other.m_data;
            this->markDirty();
        }
        return *this;
    }

    auto operator=(UBO &&other) noexcept -> UBO &
    {
        if (this != &other) {
            BindableResource<T>::operator=(std::move(other));
            m_data = std::move(other.m_data);
            this->markDirty();
            other.markClean();
        }
        return *this;
    }

    auto operator=(const T &data) -> UBO &
    {
        if (m_data != data) {
            m_data = data;
            this->markDirty();
        }
        return *this;
    }

    auto operator->() -> T *
    {
        this->markDirty();
        return &m_data;
    }

    auto operator->() const -> const T * { return &m_data; }

    auto operator*() -> T &
    {
        this->markDirty();
        return m_data;
    }

    auto operator*() const -> const T & { return m_data; }

    auto getData() -> T & override { return m_data; }

    auto getData() const -> const T & override { return m_data; }

    auto setData(const T &data) -> void
    {
        m_data = data;
        this->markDirty();
    }

    auto updateData(const T &data) -> void
    {
        m_data = data;
        this->markDirty();
    }

private:
    T m_data{};
};

} // namespace vostok::graphics