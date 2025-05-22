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

std::string vkPrimitiveTopologyToString(VkPrimitiveTopology topology);
std::string vkPolygonModeToString(VkPolygonMode mode);
std::string vkCullModeToString(VkCullModeFlags mode);
std::string vkFrontFaceToString(VkFrontFace frontFace);
std::string vkCompareOpToString(VkCompareOp compareOp);
std::string vkStencilOpToString(VkStencilOp stencilOp);
std::string vkBlendFactorToString(VkBlendFactor blendFactor);
std::string vkBlendOpToString(VkBlendOp blendOp);
std::string vkColorComponentFlagsToString(VkColorComponentFlags flags);

} // namespace vostok::graphics::vulkan::utils