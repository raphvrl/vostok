#pragma once

#include "core/logger/logger.hpp"

namespace vostok::graphics::vulkan::utils
{

inline LoggerHandle &getVulkanLogger()
{
    static LoggerHandle s_vulkanLogger = Logger::getLogger("vulkan");
    return s_vulkanLogger;
}

} // namespace vostok::graphics::vulkan::utils