#pragma once

#include "vostok/core/type.hpp"

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan::utils
{

std::string vkResultToString(VkResult result);

std::string physicalDeviceTypeToString(VkPhysicalDeviceType type);

std::string vkFormatToString(VkFormat format);

std::string vkPresentModeToString(VkPresentModeKHR mode);

std::vector<u32> vectorCharToU32(const std::vector<char> &vec);

} // namespace vostok::graphics::vulkan::utils