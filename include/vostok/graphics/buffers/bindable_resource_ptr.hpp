#pragma once

#include "vostok/graphics/buffers/bindable_resource.hpp"

#include <memory>

namespace vostok::graphics
{

template <BindableType T>
class BindableResourcePtr
{
public:
    BindableResourcePtr() = default;
    explicit BindableResourcePtr(
        std::unique_ptr<BindableResource<T>> &&resource
    )
        : m_resource(std::move(resource))
    {}

    auto operator->() -> T *
    {
        m_resource->markDirty();
        return &m_resource->getTypedData();
    }

    auto operator->() const -> const T * { return &m_resource->getTypedData(); }

    auto operator*() -> BindableResource<T> & { return *m_resource; }
    auto operator*() const -> const BindableResource<T> &
    {
        return *m_resource;
    }

    auto get() -> BindableResource<T> * { return m_resource.get(); }
    auto get() const -> const BindableResource<T> * { return m_resource.get(); }

    explicit operator bool() const { return m_resource != nullptr; }

private:
    std::unique_ptr<BindableResource<T>> m_resource;
};

template <BindableType T>
using UBOPtr = BindableResourcePtr<T>;

} // namespace vostok::graphics