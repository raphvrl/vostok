#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan::utils
{

std::string vkResultToString(VkResult result);

std::string physicalDeviceTypeToString(VkPhysicalDeviceType type);

std::string vkFormatToString(VkFormat format);

std::string vkPresentModeToString(VkPresentModeKHR mode);

} // namespace vostok::graphics::vulkan::utils