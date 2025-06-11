#pragma once

#include "vostok/core/logger/logger.hpp"

#include <vostok/utils/stl/optional.hpp>

namespace vostok::utils
{

template <typename T>
auto getValueSafe(const std::optional<T> &opt, const char *name) -> T
{
    if (!opt.has_value()) {
        Logger::critical("Attempt to access non-existent {} queue family index", name);
        throw std::runtime_error(std::string("Missing queue family index: ") + name);
    }
    return opt.value();
}

} // namespace vostok::utils