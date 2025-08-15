#include "graphics/backends/vulkan/utils/vk_utils.hpp"

#include <bit>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <volk.h>

namespace vostok::graphics::vulkan::utils
{

auto vkResultToString(VkResult result) -> std::string
{
    static const std::unordered_map<VkResult, std::string> RESULT_MAP = {
        { VK_SUCCESS, "Success" },
        { VK_NOT_READY, "Not ready" },
        { VK_TIMEOUT, "Timeout" },
        { VK_EVENT_SET, "Event set" },
        { VK_EVENT_RESET, "Event reset" },
        { VK_INCOMPLETE, "Incomplete" },
        { VK_ERROR_OUT_OF_HOST_MEMORY, "Out of host memory" },
        { VK_ERROR_OUT_OF_DEVICE_MEMORY, "Out of device memory" },
        { VK_ERROR_INITIALIZATION_FAILED, "Initialization failed" },
        { VK_ERROR_DEVICE_LOST, "Device lost" },
        { VK_ERROR_MEMORY_MAP_FAILED, "Memory map failed" },
        { VK_ERROR_LAYER_NOT_PRESENT, "Layer not present" },
        { VK_ERROR_EXTENSION_NOT_PRESENT, "Extension not present" },
        { VK_ERROR_FEATURE_NOT_PRESENT, "Feature not present" },
        { VK_ERROR_INCOMPATIBLE_DRIVER, "Incompatible driver" },
        { VK_ERROR_TOO_MANY_OBJECTS, "Too many objects" },
        { VK_ERROR_FORMAT_NOT_SUPPORTED, "Format not supported" },
        { VK_ERROR_FRAGMENTED_POOL, "Fragmented pool" },
        { VK_ERROR_UNKNOWN, "Unknown error" },
        { VK_ERROR_OUT_OF_POOL_MEMORY, "Out of pool memory" },
        { VK_ERROR_INVALID_EXTERNAL_HANDLE, "Invalid external handle" },
        { VK_ERROR_FRAGMENTATION, "Fragmentation" },
        { VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS,
          "Invalid opaque capture address" },
        { VK_ERROR_SURFACE_LOST_KHR, "Surface lost" },
        { VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "Native window in use" },
        { VK_SUBOPTIMAL_KHR, "Suboptimal" },
        { VK_ERROR_OUT_OF_DATE_KHR, "Out of date" },
        { VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "Incompatible display" },
        { VK_ERROR_VALIDATION_FAILED_EXT, "Validation failed" },
        { VK_ERROR_INVALID_SHADER_NV, "Invalid shader" },
        { VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT,
          "Invalid DRM format modifier plane layout" },
        { VK_ERROR_NOT_PERMITTED_EXT, "Not permitted" },
        { VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT,
          "Full screen exclusive mode lost" }
    };

    auto it = RESULT_MAP.find(result);
    if (it != RESULT_MAP.end()) {
        return it->second + " (" + std::to_string(static_cast<int>(result)) +
               ")";
    }

    return "Unknown Vulkan error code: " +
           std::to_string(static_cast<int>(result));
}

auto physicalDeviceTypeToString(VkPhysicalDeviceType type) -> std::string
{
    static const std::unordered_map<VkPhysicalDeviceType, std::string>
        RESULT_MAP = { { VK_PHYSICAL_DEVICE_TYPE_OTHER, "Other" },
                       { VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
                         "Integrated GPU" },
                       { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, "Discrete GPU" },
                       { VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, "Virtual GPU" },
                       { VK_PHYSICAL_DEVICE_TYPE_CPU, "CPU" } };

    auto it = RESULT_MAP.find(type);
    if (it != RESULT_MAP.end()) {
        return it->second;
    }

    return "Unknown device type (" + std::to_string(static_cast<int>(type)) +
           ")";
}

auto vkFormatToString(VkFormat format) -> std::string
{
    static const std::unordered_map<VkFormat, std::string> FORMAT_MAP = {
        { VK_FORMAT_UNDEFINED, "Undefined" },
        // 8-bit formats
        { VK_FORMAT_R4G4_UNORM_PACK8, "R4G4 UNORM PACK8" },
        { VK_FORMAT_R8_UNORM, "R8 UNORM" },
        { VK_FORMAT_R8_SNORM, "R8 SNORM" },
        { VK_FORMAT_R8_USCALED, "R8 USCALED" },
        { VK_FORMAT_R8_SSCALED, "R8 SSCALED" },
        { VK_FORMAT_R8_UINT, "R8 UINT" },
        { VK_FORMAT_R8_SINT, "R8 SINT" },
        { VK_FORMAT_R8_SRGB, "R8 SRGB" },
        // 16-bit formats
        { VK_FORMAT_R4G4B4A4_UNORM_PACK16, "R4G4B4A4 UNORM PACK16" },
        { VK_FORMAT_B4G4R4A4_UNORM_PACK16, "B4G4R4A4 UNORM PACK16" },
        { VK_FORMAT_R5G6B5_UNORM_PACK16, "R5G6B5 UNORM PACK16" },
        { VK_FORMAT_B5G6R5_UNORM_PACK16, "B5G6R5 UNORM PACK16" },
        { VK_FORMAT_R5G5B5A1_UNORM_PACK16, "R5G5B5A1 UNORM PACK16" },
        { VK_FORMAT_B5G5R5A1_UNORM_PACK16, "B5G5R5A1 UNORM PACK16" },
        { VK_FORMAT_A1R5G5B5_UNORM_PACK16, "A1R5G5B5 UNORM PACK16" },
        { VK_FORMAT_R8G8_UNORM, "R8G8 UNORM" },
        { VK_FORMAT_R8G8_SNORM, "R8G8 SNORM" },
        { VK_FORMAT_R8G8_USCALED, "R8G8 USCALED" },
        { VK_FORMAT_R8G8_SSCALED, "R8G8 SSCALED" },
        { VK_FORMAT_R8G8_UINT, "R8G8 UINT" },
        { VK_FORMAT_R8G8_SINT, "R8G8 SINT" },
        { VK_FORMAT_R8G8_SRGB, "R8G8 SRGB" },
        { VK_FORMAT_R16_UNORM, "R16 UNORM" },
        { VK_FORMAT_R16_SNORM, "R16 SNORM" },
        { VK_FORMAT_R16_USCALED, "R16 USCALED" },
        { VK_FORMAT_R16_SSCALED, "R16 SSCALED" },
        { VK_FORMAT_R16_UINT, "R16 UINT" },
        { VK_FORMAT_R16_SINT, "R16 SINT" },
        { VK_FORMAT_R16_SFLOAT, "R16 SFLOAT" },
        // 24-bit formats
        { VK_FORMAT_R8G8B8_UNORM, "R8G8B8 UNORM" },
        { VK_FORMAT_R8G8B8_SNORM, "R8G8B8 SNORM" },
        { VK_FORMAT_R8G8B8_USCALED, "R8G8B8 USCALED" },
        { VK_FORMAT_R8G8B8_SSCALED, "R8G8B8 SSCALED" },
        { VK_FORMAT_R8G8B8_UINT, "R8G8B8 UINT" },
        { VK_FORMAT_R8G8B8_SINT, "R8G8B8 SINT" },
        { VK_FORMAT_R8G8B8_SRGB, "R8G8B8 SRGB" },
        { VK_FORMAT_B8G8R8_UNORM, "B8G8R8 UNORM" },
        { VK_FORMAT_B8G8R8_SNORM, "B8G8R8 SNORM" },
        { VK_FORMAT_B8G8R8_USCALED, "B8G8R8 USCALED" },
        { VK_FORMAT_B8G8R8_SSCALED, "B8G8R8 SSCALED" },
        { VK_FORMAT_B8G8R8_UINT, "B8G8R8 UINT" },
        { VK_FORMAT_B8G8R8_SINT, "B8G8R8 SINT" },
        { VK_FORMAT_B8G8R8_SRGB, "B8G8R8 SRGB" },
        // 32-bit formats
        { VK_FORMAT_R8G8B8A8_UNORM, "R8G8B8A8 UNORM" },
        { VK_FORMAT_R8G8B8A8_SNORM, "R8G8B8A8 SNORM" },
        { VK_FORMAT_R8G8B8A8_USCALED, "R8G8B8A8 USCALED" },
        { VK_FORMAT_R8G8B8A8_SSCALED, "R8G8B8A8 SSCALED" },
        { VK_FORMAT_R8G8B8A8_UINT, "R8G8B8A8 UINT" },
        { VK_FORMAT_R8G8B8A8_SINT, "R8G8B8A8 SINT" },
        { VK_FORMAT_R8G8B8A8_SRGB, "R8G8B8A8 SRGB" },
        { VK_FORMAT_B8G8R8A8_UNORM, "B8G8R8A8 UNORM" },
        { VK_FORMAT_B8G8R8A8_SNORM, "B8G8R8A8 SNORM" },
        { VK_FORMAT_B8G8R8A8_USCALED, "B8G8R8A8 USCALED" },
        { VK_FORMAT_B8G8R8A8_SSCALED, "B8G8R8A8 SSCALED" },
        { VK_FORMAT_B8G8R8A8_UINT, "B8G8R8A8 UINT" },
        { VK_FORMAT_B8G8R8A8_SINT, "B8G8R8A8 SINT" },
        { VK_FORMAT_B8G8R8A8_SRGB, "B8G8R8A8 SRGB" },
        { VK_FORMAT_R16G16_UNORM, "R16G16 UNORM" },
        { VK_FORMAT_R16G16_SNORM, "R16G16 SNORM" },
        { VK_FORMAT_R16G16_USCALED, "R16G16 USCALED" },
        { VK_FORMAT_R16G16_SSCALED, "R16G16 SSCALED" },
        { VK_FORMAT_R16G16_UINT, "R16G16 UINT" },
        { VK_FORMAT_R16G16_SINT, "R16G16 SINT" },
        { VK_FORMAT_R16G16_SFLOAT, "R16G16 SFLOAT" },
        { VK_FORMAT_R32_UINT, "R32 UINT" },
        { VK_FORMAT_R32_SINT, "R32 SINT" },
        { VK_FORMAT_R32_SFLOAT, "R32 SFLOAT" },
        // 64-bit formats
        { VK_FORMAT_R16G16B16A16_UNORM, "R16G16B16A16 UNORM" },
        { VK_FORMAT_R16G16B16A16_SNORM, "R16G16B16A16 SNORM" },
        { VK_FORMAT_R16G16B16A16_USCALED, "R16G16B16A16 USCALED" },
        { VK_FORMAT_R16G16B16A16_SSCALED, "R16G16B16A16 SSCALED" },
        { VK_FORMAT_R16G16B16A16_UINT, "R16G16B16A16 UINT" },
        { VK_FORMAT_R16G16B16A16_SINT, "R16G16B16A16 SINT" },
        { VK_FORMAT_R16G16B16A16_SFLOAT, "R16G16B16A16 SFLOAT" },
        { VK_FORMAT_R32G32_UINT, "R32G32 UINT" },
        { VK_FORMAT_R32G32_SINT, "R32G32 SINT" },
        { VK_FORMAT_R32G32_SFLOAT, "R32G32 SFLOAT" },
        // 96-bit formats
        { VK_FORMAT_R32G32B32_UINT, "R32G32B32 UINT" },
        { VK_FORMAT_R32G32B32_SINT, "R32G32B32 SINT" },
        { VK_FORMAT_R32G32B32_SFLOAT, "R32G32B32 SFLOAT" },
        // 128-bit formats
        { VK_FORMAT_R32G32B32A32_UINT, "R32G32B32A32 UINT" },
        { VK_FORMAT_R32G32B32A32_SINT, "R32G32B32A32 SINT" },
        { VK_FORMAT_R32G32B32A32_SFLOAT, "R32G32B32A32 SFLOAT" },
        // Depth/stencil formats
        { VK_FORMAT_D16_UNORM, "D16 UNORM" },
        { VK_FORMAT_D32_SFLOAT, "D32 SFLOAT" },
        { VK_FORMAT_S8_UINT, "S8 UINT" },
        { VK_FORMAT_D16_UNORM_S8_UINT, "D16 UNORM S8 UINT" },
        { VK_FORMAT_D24_UNORM_S8_UINT, "D24 UNORM S8 UINT" },
        { VK_FORMAT_D32_SFLOAT_S8_UINT, "D32 SFLOAT S8 UINT" },
        // Compressed formats
        { VK_FORMAT_BC1_RGB_UNORM_BLOCK, "BC1 RGB UNORM BLOCK" },
        { VK_FORMAT_BC1_RGB_SRGB_BLOCK, "BC1 RGB SRGB BLOCK" },
        { VK_FORMAT_BC1_RGBA_UNORM_BLOCK, "BC1 RGBA UNORM BLOCK" },
        { VK_FORMAT_BC1_RGBA_SRGB_BLOCK, "BC1 RGBA SRGB BLOCK" },
        { VK_FORMAT_BC2_UNORM_BLOCK, "BC2 UNORM BLOCK" },
        { VK_FORMAT_BC2_SRGB_BLOCK, "BC2 SRGB BLOCK" },
        { VK_FORMAT_BC3_UNORM_BLOCK, "BC3 UNORM BLOCK" },
        { VK_FORMAT_BC3_SRGB_BLOCK, "BC3 SRGB BLOCK" },
        { VK_FORMAT_BC4_UNORM_BLOCK, "BC4 UNORM BLOCK" },
        { VK_FORMAT_BC4_SNORM_BLOCK, "BC4 SNORM BLOCK" },
        { VK_FORMAT_BC5_UNORM_BLOCK, "BC5 UNORM BLOCK" },
        { VK_FORMAT_BC5_SNORM_BLOCK, "BC5 SNORM BLOCK" },
        { VK_FORMAT_BC6H_UFLOAT_BLOCK, "BC6H UFLOAT BLOCK" },
        { VK_FORMAT_BC6H_SFLOAT_BLOCK, "BC6H SFLOAT BLOCK" },
        { VK_FORMAT_BC7_UNORM_BLOCK, "BC7 UNORM BLOCK" },
        { VK_FORMAT_BC7_SRGB_BLOCK, "BC7 SRGB BLOCK" }
    };

    auto it = FORMAT_MAP.find(format);
    if (it != FORMAT_MAP.end()) {
        return it->second;
    }

    return "Unknown format (" + std::to_string(static_cast<int>(format)) + ")";
}

auto vkPresentModeToString(VkPresentModeKHR mode) -> std::string
{
    static const std::unordered_map<VkPresentModeKHR, std::string>
        PRESENT_MODE_MAP = { { VK_PRESENT_MODE_IMMEDIATE_KHR, "Immediate" },
                             { VK_PRESENT_MODE_MAILBOX_KHR, "Mailbox" },
                             { VK_PRESENT_MODE_FIFO_KHR, "FIFO" },
                             { VK_PRESENT_MODE_FIFO_RELAXED_KHR,
                               "FIFO Relaxed" },
                             { VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR,
                               "Shared Demand Refresh" },
                             { VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR,
                               "Shared Continuous Refresh" } };

    auto it = PRESENT_MODE_MAP.find(mode);
    if (it != PRESENT_MODE_MAP.end()) {
        return it->second;
    }

    return "Unknown present mode (" + std::to_string(static_cast<int>(mode)) +
           ")";
}

auto vectorCharToU32(const std::vector<char> &vec) -> std::vector<u32>
{
    std::vector<u32> result(vec.size() / sizeof(u32));
    std::memcpy(result.data(), vec.data(), vec.size());
    return result;
}

auto vkPrimitiveTopologyToString(VkPrimitiveTopology topology) -> std::string
{
    static const std::unordered_map<VkPrimitiveTopology, std::string>
        TOPOLOGY_MAP = {
            { VK_PRIMITIVE_TOPOLOGY_POINT_LIST, "Point List" },
            { VK_PRIMITIVE_TOPOLOGY_LINE_LIST, "Line List" },
            { VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, "Line Strip" },
            { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, "Triangle List" },
            { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, "Triangle Strip" },
            { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, "Triangle Fan" },
            { VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
              "Line List With Adjacency" },
            { VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
              "Line Strip With Adjacency" },
            { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
              "Triangle List With Adjacency" },
            { VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
              "Triangle Strip With Adjacency" },
            { VK_PRIMITIVE_TOPOLOGY_PATCH_LIST, "Patch List" }
        };

    auto it = TOPOLOGY_MAP.find(topology);
    if (it != TOPOLOGY_MAP.end()) {
        return it->second;
    }

    return "Unknown primitive topology (" +
           std::to_string(static_cast<int>(topology)) + ")";
}

auto vkPolygonModeToString(VkPolygonMode mode) -> std::string
{
    static const std::unordered_map<VkPolygonMode, std::string>
        POLYGON_MODE_MAP = { { VK_POLYGON_MODE_FILL, "Fill" },
                             { VK_POLYGON_MODE_LINE, "Line" },
                             { VK_POLYGON_MODE_POINT, "Point" },
                             { VK_POLYGON_MODE_FILL_RECTANGLE_NV,
                               "Fill Rectangle" } };

    auto it = POLYGON_MODE_MAP.find(mode);
    if (it != POLYGON_MODE_MAP.end()) {
        return it->second;
    }

    return "Unknown polygon mode (" + std::to_string(static_cast<int>(mode)) +
           ")";
}

auto vkCullModeToString(VkCullModeFlags mode) -> std::string
{
    static const std::unordered_map<VkCullModeFlags, std::string>
        CULL_MODE_MAP = { { VK_CULL_MODE_NONE, "None" },
                          { VK_CULL_MODE_FRONT_BIT, "Front" },
                          { VK_CULL_MODE_BACK_BIT, "Back" },
                          { VK_CULL_MODE_FRONT_AND_BACK, "Front and Back" } };

    auto it = CULL_MODE_MAP.find(mode);
    if (it != CULL_MODE_MAP.end()) {
        return it->second;
    }

    return "Unknown cull mode (" + std::to_string(static_cast<int>(mode)) + ")";
}

auto vkFrontFaceToString(VkFrontFace frontFace) -> std::string
{
    static const std::unordered_map<VkFrontFace, std::string> FRONT_FACE_MAP = {
        { VK_FRONT_FACE_COUNTER_CLOCKWISE, "Counter Clockwise" },
        { VK_FRONT_FACE_CLOCKWISE, "Clockwise" }
    };

    auto it = FRONT_FACE_MAP.find(frontFace);
    if (it != FRONT_FACE_MAP.end()) {
        return it->second;
    }

    return "Unknown front face (" +
           std::to_string(static_cast<int>(frontFace)) + ")";
}

auto vkCompareOpToString(VkCompareOp compareOp) -> std::string
{
    static const std::unordered_map<VkCompareOp, std::string> COMPARE_OP_MAP = {
        { VK_COMPARE_OP_NEVER, "Never" },
        { VK_COMPARE_OP_LESS, "Less" },
        { VK_COMPARE_OP_EQUAL, "Equal" },
        { VK_COMPARE_OP_LESS_OR_EQUAL, "Less or Equal" },
        { VK_COMPARE_OP_GREATER, "Greater" },
        { VK_COMPARE_OP_NOT_EQUAL, "Not Equal" },
        { VK_COMPARE_OP_GREATER_OR_EQUAL, "Greater or Equal" },
        { VK_COMPARE_OP_ALWAYS, "Always" }
    };

    auto it = COMPARE_OP_MAP.find(compareOp);
    if (it != COMPARE_OP_MAP.end()) {
        return it->second;
    }

    return "Unknown compare operation (" +
           std::to_string(static_cast<int>(compareOp)) + ")";
}

auto vkStencilOpToString(VkStencilOp stencilOp) -> std::string
{
    static const std::unordered_map<VkStencilOp, std::string> STENCIL_OP_MAP = {
        { VK_STENCIL_OP_KEEP, "Keep" },
        { VK_STENCIL_OP_ZERO, "Zero" },
        { VK_STENCIL_OP_REPLACE, "Replace" },
        { VK_STENCIL_OP_INCREMENT_AND_CLAMP, "Increment and Clamp" },
        { VK_STENCIL_OP_DECREMENT_AND_CLAMP, "Decrement and Clamp" },
        { VK_STENCIL_OP_INVERT, "Invert" },
        { VK_STENCIL_OP_INCREMENT_AND_WRAP, "Increment and Wrap" },
        { VK_STENCIL_OP_DECREMENT_AND_WRAP, "Decrement and Wrap" }
    };

    auto it = STENCIL_OP_MAP.find(stencilOp);
    if (it != STENCIL_OP_MAP.end()) {
        return it->second;
    }

    return "Unknown stencil operation (" +
           std::to_string(static_cast<int>(stencilOp)) + ")";
}

auto vkBlendFactorToString(VkBlendFactor blendFactor) -> std::string
{
    static const std::unordered_map<VkBlendFactor, std::string>
        BLEND_FACTOR_MAP = {
            { VK_BLEND_FACTOR_ZERO, "Zero" },
            { VK_BLEND_FACTOR_ONE, "One" },
            { VK_BLEND_FACTOR_SRC_COLOR, "Source Color" },
            { VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, "One Minus Source Color" },
            { VK_BLEND_FACTOR_DST_COLOR, "Destination Color" },
            { VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
              "One Minus Destination Color" },
            { VK_BLEND_FACTOR_SRC_ALPHA, "Source Alpha" },
            { VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, "One Minus Source Alpha" },
            { VK_BLEND_FACTOR_DST_ALPHA, "Destination Alpha" },
            { VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
              "One Minus Destination Alpha" },
            { VK_BLEND_FACTOR_CONSTANT_COLOR, "Constant Color" },
            { VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
              "One Minus Constant Color" },
            { VK_BLEND_FACTOR_CONSTANT_ALPHA, "Constant Alpha" },
            { VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
              "One Minus Constant Alpha" }
        };

    auto it = BLEND_FACTOR_MAP.find(blendFactor);
    if (it != BLEND_FACTOR_MAP.end()) {
        return it->second;
    }

    return "Unknown blend factor (" +
           std::to_string(static_cast<int>(blendFactor)) + ")";
}

auto vkBlendOpToString(VkBlendOp blendOp) -> std::string
{
    static const std::unordered_map<VkBlendOp, std::string> BLEND_OP_MAP = {
        { VK_BLEND_OP_ADD, "Add" },
        { VK_BLEND_OP_SUBTRACT, "Subtract" },
        { VK_BLEND_OP_REVERSE_SUBTRACT, "Reverse Subtract" },
        { VK_BLEND_OP_MIN, "Min" },
        { VK_BLEND_OP_MAX, "Max" }
    };

    auto it = BLEND_OP_MAP.find(blendOp);
    if (it != BLEND_OP_MAP.end()) {
        return it->second;
    }

    return "Unknown blend operation (" +
           std::to_string(static_cast<int>(blendOp)) + ")";
}

auto vkColorComponentFlagsToString(VkColorComponentFlags flags) -> std::string
{
    static const std::unordered_map<VkColorComponentFlags, std::string>
        COLOR_COMPONENT_FLAGS_MAP = { { VK_COLOR_COMPONENT_R_BIT, "Red" },
                                      { VK_COLOR_COMPONENT_G_BIT, "Green" },
                                      { VK_COLOR_COMPONENT_B_BIT, "Blue" },
                                      { VK_COLOR_COMPONENT_A_BIT, "Alpha" } };

    std::string result;
    for (const auto &[flag, name] : COLOR_COMPONENT_FLAGS_MAP) {
        if ((flags & flag) == flag) {
            if (!result.empty()) {
                result += " | ";
            }
            result += name;
        }
    }

    if (result.empty()) {
        result = "None";
    }

    return result;
}

auto toVulkanUsage(graphics::BufferUsage usage) -> VkBufferUsageFlags
{
    VkBufferUsageFlags flags = 0;

    if ((static_cast<u32>(usage) &
         static_cast<u32>(graphics::BufferUsage::VERTEX)) != 0U) {
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }

    if ((static_cast<u32>(usage) &
         static_cast<u32>(graphics::BufferUsage::INDEX)) != 0U) {
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    if ((static_cast<u32>(usage) &
         static_cast<u32>(graphics::BufferUsage::UNIFORM)) != 0U) {
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }

    if ((static_cast<u32>(usage) &
         static_cast<u32>(graphics::BufferUsage::STORAGE)) != 0U) {
        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    if ((static_cast<u32>(usage) &
         static_cast<u32>(graphics::BufferUsage::TRANSFER_SRC)) != 0U) {
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    if ((static_cast<u32>(usage) &
         static_cast<u32>(graphics::BufferUsage::TRANSFER_DST)) != 0U) {
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    return flags;
}

auto toVmaMemoryUsage(graphics::BufferMemory memory) -> VmaMemoryUsage
{
    static const std::unordered_map<graphics::BufferMemory, VmaMemoryUsage>
        MEMORY_USAGE_MAP = {
            { graphics::BufferMemory::GPU_ONLY, VMA_MEMORY_USAGE_GPU_ONLY },
            { graphics::BufferMemory::CPU_TO_GPU, VMA_MEMORY_USAGE_CPU_TO_GPU },
            { graphics::BufferMemory::GPU_TO_CPU, VMA_MEMORY_USAGE_GPU_TO_CPU }
        };

    auto it = MEMORY_USAGE_MAP.find(memory);
    if (it != MEMORY_USAGE_MAP.end()) {
        return it->second;
    }

    return VMA_MEMORY_USAGE_GPU_ONLY;
}

auto toVulkanImageLayout(graphics::ImageLayout layout) -> VkImageLayout
{
    static const std::unordered_map<graphics::ImageLayout, VkImageLayout>
        IMAGE_LAYOUT_TO_VULKAN_MAP = {
            { graphics::ImageLayout::UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED },
            { graphics::ImageLayout::GENERAL, VK_IMAGE_LAYOUT_GENERAL },
            { graphics::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
            { graphics::ImageLayout::DEPTH_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL },
            { graphics::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
            { graphics::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { graphics::ImageLayout::TRANSFER_SRC_OPTIMAL,
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
            { graphics::ImageLayout::TRANSFER_DST_OPTIMAL,
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
            { graphics::ImageLayout::PRESENT_SRC_KHR,
              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }
        };

    auto it = IMAGE_LAYOUT_TO_VULKAN_MAP.find(layout);
    if (it != IMAGE_LAYOUT_TO_VULKAN_MAP.end()) {
        return it->second;
    }

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

auto toVulkanImageAspectFlags(graphics::ImageFormat format)
    -> VkImageAspectFlags
{
    static const std::unordered_map<graphics::ImageFormat, VkImageAspectFlags>
        IMAGE_ASPECT_FLAGS_MAP = {
            { graphics::ImageFormat::R8G8B8A8_UNORM,
              VK_IMAGE_ASPECT_COLOR_BIT },
            { graphics::ImageFormat::R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT },
            { graphics::ImageFormat::B8G8R8A8_UNORM,
              VK_IMAGE_ASPECT_COLOR_BIT },
            { graphics::ImageFormat::B8G8R8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT },
            { graphics::ImageFormat::R32G32B32A32_SFLOAT,
              VK_IMAGE_ASPECT_COLOR_BIT },
        };

    auto it = IMAGE_ASPECT_FLAGS_MAP.find(format);
    if (it != IMAGE_ASPECT_FLAGS_MAP.end()) {
        return it->second;
    }

    return VK_IMAGE_ASPECT_COLOR_BIT;
}

auto toVulkanAspectFlags(graphics::ImageAspectFlags aspectFlags)
    -> VkImageAspectFlags
{
    static const std::
        unordered_map<graphics::ImageAspectFlags, VkImageAspectFlags>
            ASPECT_FLAGS_MAP = { { graphics::ImageAspectFlags::COLOR,
                                   VK_IMAGE_ASPECT_COLOR_BIT },
                                 { graphics::ImageAspectFlags::DEPTH,
                                   VK_IMAGE_ASPECT_DEPTH_BIT },
                                 { graphics::ImageAspectFlags::STENCIL,
                                   VK_IMAGE_ASPECT_STENCIL_BIT },
                                 { graphics::ImageAspectFlags::DEPTH_STENCIL,
                                   VK_IMAGE_ASPECT_DEPTH_BIT |
                                       VK_IMAGE_ASPECT_STENCIL_BIT },
                                 { graphics::ImageAspectFlags::METADATA,
                                   VK_IMAGE_ASPECT_METADATA_BIT } };

    auto it = ASPECT_FLAGS_MAP.find(aspectFlags);

    if (it != ASPECT_FLAGS_MAP.end()) {
        return it->second;
    }

    return VK_IMAGE_ASPECT_COLOR_BIT;
}

auto setDebugObjectName(
    VkDevice device,
    VkObjectType objectType,
    u64 objectHandle,
    const std::string &name
) -> bool
{
    if (name.empty()) {
        return false;
    }

    auto func = std::bit_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT")
    );

    if (func == nullptr) {
        return false;
    }

    VkDebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = objectHandle;
    nameInfo.pObjectName = name.c_str();

    VkResult result = func(device, &nameInfo);
    return result == VK_SUCCESS;
}

auto toVulkanFormat(graphics::ImageFormat format) -> VkFormat
{
    static const std::unordered_map<graphics::ImageFormat, VkFormat>
        IMAGE_FORMAT_TO_VULKAN_MAP = {
            { graphics::ImageFormat::R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM },
            { graphics::ImageFormat::R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB },
            { graphics::ImageFormat::B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM },
            { graphics::ImageFormat::B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB },
            { graphics::ImageFormat::R32G32B32A32_SFLOAT,
              VK_FORMAT_R32G32B32A32_SFLOAT },
            { graphics::ImageFormat::D32_SFLOAT, VK_FORMAT_D32_SFLOAT },
            { graphics::ImageFormat::D24_UNORM_S8_UINT,
              VK_FORMAT_D24_UNORM_S8_UINT }
        };

    auto it = IMAGE_FORMAT_TO_VULKAN_MAP.find(format);
    if (it != IMAGE_FORMAT_TO_VULKAN_MAP.end()) {
        return it->second;
    }

    return VK_FORMAT_UNDEFINED;
}

auto toVulkanSampleCount(graphics::SampleCount samples) -> VkSampleCountFlagBits
{
    static const std::
        unordered_map<graphics::SampleCount, VkSampleCountFlagBits>
            SAMPLE_COUNT_TO_VULKAN_MAP = {
                { graphics::SampleCount::COUNT_1, VK_SAMPLE_COUNT_1_BIT },
                { graphics::SampleCount::COUNT_2, VK_SAMPLE_COUNT_2_BIT },
                { graphics::SampleCount::COUNT_4, VK_SAMPLE_COUNT_4_BIT },
                { graphics::SampleCount::COUNT_8, VK_SAMPLE_COUNT_8_BIT },
                { graphics::SampleCount::COUNT_16, VK_SAMPLE_COUNT_16_BIT }
            };

    auto it = SAMPLE_COUNT_TO_VULKAN_MAP.find(samples);
    if (it != SAMPLE_COUNT_TO_VULKAN_MAP.end()) {
        return it->second;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

auto toVulkanImageUsage(graphics::ImageUsage usage) -> VkImageUsageFlags
{
    static const std::unordered_map<graphics::ImageUsage, VkImageUsageFlags>
        IMAGE_USAGE_TO_VULKAN_MAP = {
            { graphics::ImageUsage::TRANSFER_SRC,
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT },
            { graphics::ImageUsage::TRANSFER_DST,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT },
            { graphics::ImageUsage::SAMPLED, VK_IMAGE_USAGE_SAMPLED_BIT },
            { graphics::ImageUsage::STORAGE, VK_IMAGE_USAGE_STORAGE_BIT },
        };

    auto it = IMAGE_USAGE_TO_VULKAN_MAP.find(usage);
    if (it != IMAGE_USAGE_TO_VULKAN_MAP.end()) {
        return it->second;
    }

    return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
}

auto getRequiredAlignment(graphics::BufferUsage usage) -> size_t
{
    size_t requiredAlignment = 1;

    if ((static_cast<u32>(usage) &
         static_cast<u32>(graphics::BufferUsage::UNIFORM)) != 0U) {
        requiredAlignment = std::max(requiredAlignment, size_t(256));
    }

    if ((static_cast<u32>(usage) &
         static_cast<u32>(graphics::BufferUsage::STORAGE)) != 0U) {
        requiredAlignment = std::max(requiredAlignment, size_t(16));
    }

    if (((static_cast<u32>(usage) &
          static_cast<u32>(graphics::BufferUsage::VERTEX)) != 0U) ||
        ((static_cast<u32>(usage) &
          static_cast<u32>(graphics::BufferUsage::INDEX)) != 0U)) {
        requiredAlignment = std::max(requiredAlignment, size_t(4));
    }

    return requiredAlignment;
}

auto vertexFormatToVulkan(graphics::VertexFormat format) -> VkFormat
{
    static const std::unordered_map<graphics::VertexFormat, VkFormat>
        VERTEX_FORMAT_TO_VULKAN_MAP = {
            { graphics::VertexFormat::FLOAT1, VK_FORMAT_R32_SFLOAT },
            { graphics::VertexFormat::FLOAT2, VK_FORMAT_R32G32_SFLOAT },
            { graphics::VertexFormat::FLOAT3, VK_FORMAT_R32G32B32_SFLOAT },
            { graphics::VertexFormat::FLOAT4, VK_FORMAT_R32G32B32A32_SFLOAT },
            { graphics::VertexFormat::INT1, VK_FORMAT_R32_SINT },
            { graphics::VertexFormat::INT2, VK_FORMAT_R32G32_SINT },
            { graphics::VertexFormat::INT3, VK_FORMAT_R32G32B32_SINT },
            { graphics::VertexFormat::INT4, VK_FORMAT_R32G32B32A32_SINT },
            { graphics::VertexFormat::UINT1, VK_FORMAT_R32_UINT },
            { graphics::VertexFormat::UINT2, VK_FORMAT_R32G32_UINT },
            { graphics::VertexFormat::UINT3, VK_FORMAT_R32G32B32_UINT },
            { graphics::VertexFormat::UINT4, VK_FORMAT_R32G32B32A32_UINT }
        };

    auto it = VERTEX_FORMAT_TO_VULKAN_MAP.find(format);
    if (it != VERTEX_FORMAT_TO_VULKAN_MAP.end()) {
        return it->second;
    }

    return VK_FORMAT_R32_SFLOAT;
}

auto loadShaderFile(const fs::path &path)
    -> std::expected<std::vector<u32>, std::string>
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected("Cannot open shader file: " + path.string());
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (static_cast<size_t>(size) % sizeof(u32) != 0) {
        return std::unexpected("Shader file size is not a multiple of 4 bytes");
    }

    std::vector<u32> shaderCode(static_cast<size_t>(size) / sizeof(u32));
    file.read(std::bit_cast<char *>(shaderCode.data()), size);

    if (file.fail()) {
        return std::unexpected("Failed to read shader file: " + path.string());
    }

    return shaderCode;
}

auto createShaderModule(VkDevice device, const std::vector<u32> &code)
    -> std::expected<VkShaderModule, std::string>
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.codeSize = code.size() * sizeof(u32);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
        VK_SUCCESS) {
        return std::unexpected("Failed to create shader module");
    }

    return shaderModule;
}

auto createShaderModule(VkDevice device, const std::filesystem::path &path)
    -> std::expected<VkShaderModule, std::string>
{
    auto shaderCode = loadShaderFile(path);
    if (!shaderCode) {
        return std::unexpected(shaderCode.error());
    }

    return createShaderModule(device, *shaderCode);
}

auto createPipelineLayout(
    VkDevice device,
    const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts,
    const std::vector<VkPushConstantRange> &pushConstantRanges
) -> std::expected<VkPipelineLayout, std::string>
{
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;
    layoutInfo.flags = 0;
    layoutInfo.setLayoutCount = static_cast<u32>(descriptorSetLayouts.size());
    layoutInfo.pSetLayouts = descriptorSetLayouts.data();
    layoutInfo.pushConstantRangeCount =
        static_cast<u32>(pushConstantRanges.size());
    layoutInfo.pPushConstantRanges = pushConstantRanges.data();

    VkPipelineLayout layout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout) !=
        VK_SUCCESS) {
        return std::unexpected("Failed to create pipeline layout");
    }

    return layout;
}

auto createGraphicsPipeline(
    VkDevice device,
    const PipelineCreateInfo &info,
    const std::vector<VkPipelineShaderStageCreateInfo> &shaderStages,
    VkPipelineLayout layout,
    VkRenderPass renderPass,
    u32 subpass,
    VkFormat colorAttachmentFormat
) -> std::expected<VkPipeline, std::string>
{
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription bindingDescription{};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    if (info.vertexLayout.has_value()) {
        const auto &vertexLayout = info.vertexLayout.value();

        bindingDescription.binding = 0;
        bindingDescription.stride = vertexLayout.stride;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        attributeDescriptions.reserve(vertexLayout.attributes.size());

        for (const auto &attr : vertexLayout.attributes) {
            VkVertexInputAttributeDescription desc{};
            desc.binding = 0;
            desc.location = attr.location;
            desc.format = vertexFormatToVulkan(attr.format);
            desc.offset = attr.offset;
            attributeDescriptions.push_back(desc);
        }

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<u32>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions =
            attributeDescriptions.data();
    } else {
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VkPrimitiveTopology(info.primitiveTopology);
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VkPolygonMode(info.polygonMode);
    rasterizer.cullMode = VkCullModeFlags(info.cullMode);
    rasterizer.frontFace = VkFrontFace(info.frontFace);
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0F;
    rasterizer.depthBiasClamp = 0.0F;
    rasterizer.depthBiasSlopeFactor = 0.0F;
    rasterizer.lineWidth = info.lineWidth;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.minSampleShading = 1.0F;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = info.depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = info.depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VkCompareOp(info.depthCompareOp);
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = info.stencilTest ? VK_TRUE : VK_FALSE;
    depthStencil.front = { .failOp = VkStencilOp(info.stencilFailOp),
                           .passOp = VkStencilOp(info.stencilPassOp),
                           .depthFailOp = VkStencilOp(info.stencilDepthFailOp),
                           .compareOp = VK_COMPARE_OP_ALWAYS,
                           .compareMask = 0,
                           .writeMask = 0,
                           .reference = 0 };
    depthStencil.back = { .failOp = VkStencilOp(info.stencilFailOp),
                          .passOp = VkStencilOp(info.stencilPassOp),
                          .depthFailOp = VkStencilOp(info.stencilDepthFailOp),
                          .compareOp = VK_COMPARE_OP_ALWAYS,
                          .compareMask = 0,
                          .writeMask = 0,
                          .reference = 0 };
    depthStencil.minDepthBounds = 0.0F;
    depthStencil.maxDepthBounds = 1.0F;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = info.blend ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor =
        VkBlendFactor(info.srcColorBlendFactor);
    colorBlendAttachment.dstColorBlendFactor =
        VkBlendFactor(info.dstColorBlendFactor);
    colorBlendAttachment.colorBlendOp = VkBlendOp(info.colorBlendOp);
    colorBlendAttachment.srcAlphaBlendFactor =
        VkBlendFactor(info.srcAlphaBlendFactor);
    colorBlendAttachment.dstAlphaBlendFactor =
        VkBlendFactor(info.dstAlphaBlendFactor);
    colorBlendAttachment.alphaBlendOp = VkBlendOp(info.alphaBlendOp);
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0F;
    colorBlending.blendConstants[1] = 0.0F;
    colorBlending.blendConstants[2] = 0.0F;
    colorBlending.blendConstants[3] = 0.0F;

    std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,
                                                    VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<u32>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = subpass;

    VkPipelineRenderingCreateInfo renderingInfo{};
    if (renderPass == VK_NULL_HANDLE &&
        colorAttachmentFormat != VK_FORMAT_UNDEFINED) {
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.pNext = nullptr;
        renderingInfo.viewMask = 0;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &colorAttachmentFormat;
        renderingInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
        renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
        pipelineInfo.pNext = &renderingInfo;
    }

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult result = vkCreateGraphicsPipelines(
        device,
        nullptr,
        1,
        &pipelineInfo,
        nullptr,
        &pipeline
    );
    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to create graphics pipeline: " + vkResultToString(result)
        );
    }

    return pipeline;
}

} // namespace vostok::graphics::vulkan::utils