#pragma once

#include "vostok/graphics/buffers/bindable_resource.hpp"

#include <memory>
#include <span>
#include <stdexcept>

namespace vostok::graphics
{

template <typename T>
concept SSBOType =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

template <SSBOType T>
class SSBOImpl : public BindableResource
{
public:
    using DataType = T;

    SSBOImpl() = default;
    ~SSBOImpl() override = default;

    SSBOImpl(const SSBOImpl &other) noexcept = default;
    SSBOImpl(SSBOImpl &&other) noexcept = default;
    auto operator=(const SSBOImpl &other) -> SSBOImpl & = default;
    auto operator=(SSBOImpl &&other) noexcept -> SSBOImpl & = default;

    explicit SSBOImpl(const std::span<const T> &data)
        : BindableResource(),
          m_data(data.begin(), data.end())
    {
        markDirty();
    }

    // NOLINTNEXTLINE(readability-identifier-naming)
    void push_back(const T &data)
    {
        m_data.push_back(data);
        markDirty();
    }

    auto clear() -> void
    {
        m_data.clear();
        markDirty();
    }

    auto resize(size_t newSize) -> void
    {
        m_data.resize(newSize);
        markDirty();
    }

    [[nodiscard]] auto size() const -> size_t { return m_data.size(); }
    [[nodiscard]] auto capacity() const -> size_t { return m_data.capacity(); }
    [[nodiscard]] auto empty() const -> bool { return m_data.empty(); }

    [[nodiscard]] auto data() -> T * { return m_data.data(); }
    [[nodiscard]] auto data() const -> const T * { return m_data.data(); }

    [[nodiscard]] auto getRawData() const noexcept -> const void * override
    {
        return m_data.data();
    }

    [[nodiscard]] auto getDataSize() const noexcept -> size_t override
    {
        return m_data.size() * sizeof(T);
    }

private:
    std::vector<T> m_data;
};

template <SSBOType T>
class SSBO
{
public:
    SSBO() = default;
    ~SSBO() = default;

    explicit SSBO(std::unique_ptr<SSBOImpl<T>> &&ssbo)
        : m_ssbo(std::move(ssbo))
    {}

    SSBO(SSBO &&other) noexcept = default;
    auto operator=(SSBO &&other) noexcept -> SSBO & = default;
    SSBO(const SSBO &) noexcept = delete;
    auto operator=(const SSBO &) noexcept -> SSBO & = delete;

    // NOLINTNEXTLINE(readability-identifier-naming)
    auto push_back(const T &value) -> void
    {
        if (!m_ssbo) {
            throw std::runtime_error("SSBO is null");
        }
        m_ssbo->push_back(value);
    }

    auto clear() -> void
    {
        if (!m_ssbo) {
            throw std::runtime_error("SSBO is null");
        }
        m_ssbo->clear();
    }

    auto resize(size_t newSize) -> void
    {
        if (!m_ssbo) {
            throw std::runtime_error("SSBO is null");
        }
        m_ssbo->resize(newSize);
    }

    [[nodiscard]] auto size() const -> size_t
    {
        return m_ssbo ? m_ssbo->size() : 0;
    }

    auto data() -> T *
    {
        if (!m_ssbo) {
            throw std::runtime_error("SSBO is null");
        }
        return m_ssbo->data();
    }

    auto data() const -> const T *
    {
        if (!m_ssbo) {
            throw std::runtime_error("SSBO is null");
        }
        return m_ssbo->data();
    }

    auto operator->() -> SSBOImpl<T> *
    {
        if (!m_ssbo) {
            throw std::runtime_error("SSBO is null");
        }
        return m_ssbo.get();
    }

    auto operator->() const -> const SSBOImpl<T> *
    {
        if (!m_ssbo) {
            throw std::runtime_error("SSBO is null");
        }
        return m_ssbo.get();
    }

    auto get() -> SSBOImpl<T> * { return m_ssbo.get(); }
    auto get() const -> const SSBOImpl<T> * { return m_ssbo.get(); }

    explicit operator bool() const { return m_ssbo != nullptr; }

private:
    std::unique_ptr<SSBOImpl<T>> m_ssbo;
};

} // namespace vostok::graphics