#pragma once

#include "vostok/core/type.hpp"

#include <functional>

namespace vostok::graphics
{
class BindableResource
{
public:
    using DirtyCallback = std::function<void(const BindableResource *)>;

    BindableResource() = default;
    virtual ~BindableResource() = default;

    BindableResource(const BindableResource &other) noexcept = delete;
    BindableResource(BindableResource &&other) noexcept = delete;
    auto operator=(const BindableResource &other) noexcept
        -> BindableResource & = delete;
    auto operator=(BindableResource &&other) noexcept
        -> BindableResource & = delete;

    [[nodiscard]] auto isDirty() const noexcept -> bool { return m_isDirty; }

    void markDirty() noexcept
    {
        m_isDirty = true;

        if (m_dirtyCallback) {
            m_dirtyCallback(this);
        }
    }

    void clearDirty() noexcept { m_isDirty = false; }

    [[nodiscard]] auto getBindlessIndex() const noexcept -> u32
    {
        return m_bindlessIndex;
    }

    void setBindlessIndex(u32 index) noexcept { m_bindlessIndex = index; }

    void setDirtyCallback(DirtyCallback callback)
    {
        m_dirtyCallback = std::move(callback);
    }

    [[nodiscard]] auto hasDirtyCallback() const noexcept -> bool
    {
        return static_cast<bool>(m_dirtyCallback);
    }

    [[nodiscard]] virtual auto getDataSize() const noexcept -> size_t = 0;
    [[nodiscard]] virtual auto getRawData() const -> const void * = 0;

private:
    bool m_isDirty = true;
    mutable u32 m_bindlessIndex = UINT32_MAX;
    DirtyCallback m_dirtyCallback;
};

} // namespace vostok::graphics