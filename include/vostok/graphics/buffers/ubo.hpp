#pragma once

#include "vostok/graphics/buffers/bindable_resource.hpp"

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
        : BindableResource<T>()
    {
        this->getTypedData() = data;
        this->markDirty();
    }

    explicit UBO(T &&data)
        : BindableResource<T>()
    {
        this->getTypedData() = std::move(data);
        this->markDirty();
    }

    UBO(const UBO &other)
        : BindableResource<T>(other)
    {
        this->markDirty();
    }

    UBO(UBO &&other) noexcept
        : BindableResource<T>(std::move(other))
    {
        this->markDirty();
        other.clearDirty();
    }

    auto operator=(const UBO &other) -> UBO &
    {
        if (this != &other) {
            BindableResource<T>::operator=(other);
            this->markDirty();
        }
        return *this;
    }

    auto operator=(UBO &&other) noexcept -> UBO &
    {
        if (this != &other) {
            BindableResource<T>::operator=(std::move(other));
            this->markDirty();
        }
        return *this;
    }

    auto operator=(const T &data) -> UBO &
    {
        if (this->getTypedData() != data) {
            this->getTypedData() = data;
            this->markDirty();
        }
        return *this;
    }
};

} // namespace vostok::graphics