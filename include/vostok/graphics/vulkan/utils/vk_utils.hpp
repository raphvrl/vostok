#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan::utils
{

std::string vkResultToString(VkResult result);

std::string physicalDeviceTypeToString(VkPhysicalDeviceType type);

} // namespace vostok::graphics::vulkan::utils