#pragma once

#include "vostok/core/type.hpp"

#include <functional>
#include <type_traits>

namespace vostok::graphics
{

template <typename T>
concept BindableType =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> &&
    sizeof(T) <= static_cast<u32>(64 * 1024);

class BindableResourceBase
{
public:
    BindableResourceBase() = default;
    virtual ~BindableResourceBase() = default;

    BindableResourceBase(const BindableResourceBase &other) = delete;
    BindableResourceBase(BindableResourceBase &&other) noexcept = delete;
    auto operator=(const BindableResourceBase &other)
        -> BindableResourceBase & = delete;
    auto operator=(BindableResourceBase &&other) noexcept
        -> BindableResourceBase & = delete;

    [[nodiscard]] virtual auto isDirty() const noexcept -> bool = 0;
    virtual void clearDirty() noexcept = 0;
    [[nodiscard]] virtual auto getBindlessIndex() const noexcept -> u32 = 0;
    virtual void setBindlessIndex(u32 index) noexcept = 0;
    [[nodiscard]] virtual auto getData() const noexcept -> const void * = 0;
    [[nodiscard]] virtual auto getSize() const noexcept -> size_t = 0;
};

template <BindableType T>
class BindableResource : public BindableResourceBase
{
public:
    using DataType = T;
    using DirtyCallback = std::function<void(const BindableResource<T> *)>;

    BindableResource() = default;
    virtual ~BindableResource() = default;

    BindableResource(const BindableResource &other) noexcept = default;

    BindableResource(BindableResource &&other) noexcept = default;

    auto operator=(const BindableResource &other) -> BindableResource &
    {
        if (this != &other) {
            m_isDirty = true;
        }
        return *this;
    }

    auto operator=(BindableResource &&other) noexcept -> BindableResource &
    {
        if (this != &other) {
            m_isDirty = other.m_isDirty;
            other.m_isDirty = false;
        }
        return *this;
    }

    [[nodiscard]] auto isDirty() const noexcept -> bool override
    {
        return m_isDirty;
    }

    void markDirty() noexcept
    {
        m_isDirty = true;

        if (m_dirtyCallback) {
            m_dirtyCallback(this);
        }
    }

    void clearDirty() noexcept override { m_isDirty = false; }

    [[nodiscard]] auto getBindlessIndex() const noexcept -> u32 override
    {
        return m_bindlessIndex;
    }

    void setBindlessIndex(u32 index) noexcept override
    {
        m_bindlessIndex = index;
    }

    [[nodiscard]] auto getData() const noexcept -> const void * override
    {
        return &m_data;
    }

    [[nodiscard]] auto getSize() const noexcept -> size_t override
    {
        return sizeof(T);
    }

    [[nodiscard]] auto getTypedData() const noexcept -> const T &
    {
        return m_data;
    }

    [[nodiscard]] auto getTypedData() noexcept -> T & { return m_data; }

    [[nodiscard]] auto getDataSize() const noexcept -> size_t
    {
        return sizeof(T);
    }

    void setDirtyCallback(DirtyCallback callback)
    {
        m_dirtyCallback = std::move(callback);
    }

    [[nodiscard]] auto hasDirtyCallback() const noexcept -> bool
    {
        return static_cast<bool>(m_dirtyCallback);
    }

    template <BindableType U>
    [[nodiscard]] auto as() const noexcept -> const BindableResource<U> *
    {
        return dynamic_cast<const BindableResource<U> *>(this);
    }

private:
    T m_data{};
    bool m_isDirty = true;
    mutable u32 m_bindlessIndex = UINT32_MAX;
    DirtyCallback m_dirtyCallback;
};

} // namespace vostok::graphics