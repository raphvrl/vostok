#pragma once

#include <optional>

namespace vostok::utils
{

template <typename T>
T getValueSafe(const std::optional<T> &opt, const char *name);

} // namespace vostok::utils

#include "vostok/utils/stl/optional.inl"