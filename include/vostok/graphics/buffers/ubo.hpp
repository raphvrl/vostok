#pragma once

#include "vostok/graphics/buffers/bindable_resource.hpp"

#include <memory>
#include <type_traits>

namespace vostok::graphics
{

template <typename T>
concept UBOType =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> &&
    sizeof(T) <= static_cast<u32>(64 * 1024); // 64KB max pour UBO

template <UBOType T>
class UBOImpl : public BindableResource
{
public:
    using DataType = T;

    UBOImpl() = default;
    ~UBOImpl() override = default;

    UBOImpl(const UBOImpl &other) noexcept = default;
    UBOImpl(UBOImpl &&other) noexcept = default;
    auto operator=(const UBOImpl &other) -> UBOImpl & = default;
    auto operator=(UBOImpl &&other) noexcept -> UBOImpl & = default;

    explicit UBOImpl(const T &data)
        : BindableResource(),
          m_data(data)
    {
        markDirty();
    }

    explicit UBOImpl(T &&data)
        : BindableResource(),
          m_data(std::move(data))
    {
        markDirty();
    }

    auto operator=(const T &data) -> UBOImpl &
    {
        if (m_data != data) {
            m_data = data;
            markDirty();
        }
        return *this;
    }

    auto setUniform(const T &data) -> UBOImpl & { return operator=(data); }

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
class UBO
{
public:
    UBO() = default;
    ~UBO() = default;

    explicit UBO(std::unique_ptr<UBOImpl<T>> &&ubo)
        : m_ubo(std::move(ubo))
    {}

    UBO(UBO &&other) noexcept = default;
    auto operator=(UBO &&other) noexcept -> UBO & = default;
    UBO(const UBO &) noexcept = delete;
    auto operator=(const UBO &) noexcept -> UBO & = delete;

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

    auto get() -> UBOImpl<T> * { return m_ubo.get(); }
    auto get() const -> const UBOImpl<T> * { return m_ubo.get(); }

    explicit operator bool() const { return m_ubo != nullptr; }

    auto release() -> std::unique_ptr<UBOImpl<T>> { return std::move(m_ubo); }

private:
    std::unique_ptr<UBOImpl<T>> m_ubo;
};

} // namespace vostok::graphics