#pragma once

#include "vostok/core/type.hpp"
#include "vostok/graphics/buffer.hpp"
#include "vostok/graphics/pipeline.hpp"

#include <filesystem>
#include <string>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace fs = std::filesystem;

namespace vostok::graphics::vulkan::utils
{

auto vkResultToString(VkResult result) -> std::string;

auto physicalDeviceTypeToString(VkPhysicalDeviceType type) -> std::string;

auto vkFormatToString(VkFormat format) -> std::string;

auto vkPresentModeToString(VkPresentModeKHR mode) -> std::string;

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

auto loadShaderFile(const fs::path &path)
    -> std::expected<std::vector<u32>, std::string>;

auto createShaderModule(VkDevice device, const std::vector<u32> &code)
    -> std::expected<VkShaderModule, std::string>;

auto createShaderModule(VkDevice device, const fs::path &path)
    -> std::expected<VkShaderModule, std::string>;

auto createPipelineLayout(
    VkDevice device,
    const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts,
    const std::vector<VkPushConstantRange> &pushConstantRanges
) -> std::expected<VkPipelineLayout, std::string>;

auto createGraphicsPipeline(
    VkDevice device,
    const PipelineCreateInfo &info,
    const std::vector<VkPipelineShaderStageCreateInfo> &shaderStages,
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    u32 subpass,
    VkFormat colorAttachmentFormat
) -> std::expected<VkPipeline, std::string>;

} // namespace vostok::graphics::vulkan::utils