#pragma once

#include "core/logger/logger.hpp"

namespace vostok::graphics::vulkan::utils
{

inline auto getVulkanLogger() -> LoggerHandle &
{
    static LoggerHandle s_vulkanLogger = Logger::getLogger("vulkan");
    return s_vulkanLogger;
}

} // namespace vostok::graphics::vulkan::utils