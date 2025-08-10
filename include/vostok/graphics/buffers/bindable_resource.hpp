#pragma once

#include "vostok/core/type.hpp"

#include <type_traits>

namespace vostok::graphics
{

template <typename T>
concept BindableType =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> &&
    sizeof(T) <= static_cast<u32>(64 * 1024);

template <BindableType T>
class BindableResource
{
public:
    using DataType = T;

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

    [[nodiscard]] auto isDirty() const noexcept -> bool { return m_isDirty; }
    void markDirty() noexcept { m_isDirty = true; }
    void clearDirty() noexcept { m_isDirty = false; }

    [[nodiscard]] auto getDataSize() const noexcept -> size_t
    {
        return sizeof(T);
    }

private:
    bool m_isDirty = true;
    mutable u32 m_bindlessIndex = UINT32_MAX;
};

} // namespace vostok::graphics