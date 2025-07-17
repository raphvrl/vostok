#pragma once

#include "vostok/core/type.hpp"
#include "vostok/graphics/buffer.hpp"

#include <string>
#include <vector>
#include <vk_mem_alloc.h>
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

auto toVulkanUsage(graphics::BufferUsage usage) -> VkBufferUsageFlags;
auto toVmaMemoryUsage(graphics::BufferMemory memory) -> VmaMemoryUsage;

auto setDebugObjectName(
    VkDevice device,
    VkObjectType objectType,
    u64 objectHandle,
    const std::string &name
) -> bool;

auto getRequiredAlignment(graphics::BufferUsage usage) -> size_t;

} // namespace vostok::graphics::vulkan::utils