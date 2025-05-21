#include "graphics/vulkan/utils/vk_utils.hpp"

#include <cstring>
#include <unordered_map>


namespace vostok::graphics::vulkan::utils
{

std::string vkResultToString(VkResult result)
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
        { VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "Invalid opaque capture address" },
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
        { VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, "Full screen exclusive mode lost" }
    };

    auto it = RESULT_MAP.find(result);
    if (it != RESULT_MAP.end()) {
        return it->second + " (" + std::to_string(static_cast<int>(result)) + ")";
    }

    return "Unknown Vulkan error code: " + std::to_string(static_cast<int>(result));
}

std::string physicalDeviceTypeToString(VkPhysicalDeviceType type)
{
    static const std::unordered_map<VkPhysicalDeviceType, std::string> RESULT_MAP = {
        { VK_PHYSICAL_DEVICE_TYPE_OTHER, "Other" },
        { VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, "Integrated GPU" },
        { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, "Discrete GPU" },
        { VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, "Virtual GPU" },
        { VK_PHYSICAL_DEVICE_TYPE_CPU, "CPU" }
    };

    auto it = RESULT_MAP.find(type);
    if (it != RESULT_MAP.end()) {
        return it->second;
    }

    return "Unknown device type (" + std::to_string(static_cast<int>(type)) + ")";
}

std::string vkFormatToString(VkFormat format)
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

std::string vkPresentModeToString(VkPresentModeKHR mode)
{
    static const std::unordered_map<VkPresentModeKHR, std::string> PRESENT_MODE_MAP = {
        { VK_PRESENT_MODE_IMMEDIATE_KHR, "Immediate" },
        { VK_PRESENT_MODE_MAILBOX_KHR, "Mailbox" },
        { VK_PRESENT_MODE_FIFO_KHR, "FIFO" },
        { VK_PRESENT_MODE_FIFO_RELAXED_KHR, "FIFO Relaxed" },
        { VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR, "Shared Demand Refresh" },
        { VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR, "Shared Continuous Refresh" }
    };

    auto it = PRESENT_MODE_MAP.find(mode);
    if (it != PRESENT_MODE_MAP.end()) {
        return it->second;
    }

    return "Unknown present mode (" + std::to_string(static_cast<int>(mode)) + ")";
}

std::vector<u32> vectorCharToU32(const std::vector<char> &vec)
{
    std::vector<u32> result(vec.size() / sizeof(u32));
    std::memcpy(result.data(), vec.data(), vec.size());
    return result;
}

} // namespace vostok::graphics::vulkan::utils