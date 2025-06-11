#pragma once

#include "vostok/core/type.hpp"

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan::utils
{

auto vkResultToString(VkResult result) -> std::string;

auto physicalDeviceTypeToString(VkPhysicalDeviceType type) -> std::string;

auto vkFormatToString(VkFormat format) -> std::string;

auto vkPresentModeToString(VkPresentModeKHR mode) -> std::string;

auto vectorCharToU32(const std::vector<char> &vec) -> std::vector<u32>;

auto vkPrimitiveTopologyToString(VkPrimitiveTopology topology) -> std::string;
auto vkPolygonModeToString(VkPolygonMode mode) -> std::string;
auto vkCullModeToString(VkCullModeFlags mode) -> std::string;
auto vkFrontFaceToString(VkFrontFace frontFace) -> std::string;
auto vkCompareOpToString(VkCompareOp compareOp) -> std::string;
auto vkStencilOpToString(VkStencilOp stencilOp) -> std::string;
auto vkBlendFactorToString(VkBlendFactor blendFactor) -> std::string;
auto vkBlendOpToString(VkBlendOp blendOp) -> std::string;
auto vkColorComponentFlagsToString(VkColorComponentFlags flags) -> std::string;

} // namespace vostok::graphics::vulkan::utils