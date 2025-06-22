#pragma once

#include <optional>

namespace vostok::utils
{

template <typename T>
auto getValueSafe(const std::optional<T> &opt, const char *name) -> T;

} // namespace vostok::utils

#include "vostok/utils/stl/optional.inl"