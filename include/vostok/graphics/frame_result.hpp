#pragma once

#include "vostok/core/type.hpp"

#include <string>

namespace vostok::graphics
{

enum class GPUError : u8
{
    SUCCESS,
    DEVICE_LOST,
    OUT_OF_MEMORY,
    INVALID_OPERATION,
    OTHER_ERROR
};

enum class FrameError : u8
{
    SUCCESS,
    SWAPCHAIN_OUT_OF_DATE,
    SURFACE_LOST,
    DEVICE_LOST,
    OTHER_ERROR
};

enum class SwapchainError : u8
{
    SUCCESS,
    OUT_OF_DATE,
    SURFACE_LOST,
    DEVICE_LOST,
    INVALID_FORMAT,
    OTHER_ERROR
};

struct ErrorInfo
{
    GPUError type;
    std::string message;
    std::string context;
};

struct FrameErrorInfo
{
    FrameError type;
    std::string message;
    std::string context;
};

struct SwapchainErrorInfo
{
    SwapchainError type;
    std::string message;
    std::string context;
};

inline auto swapchainErrorToString(const SwapchainErrorInfo &error)
    -> std::string
{
    return error.context + ": " + error.message;
}

inline auto frameErrorToString(const FrameErrorInfo &error) -> std::string
{
    return error.context + ": " + error.message;
}

inline auto gpuErrorToString(const ErrorInfo &error) -> std::string
{
    return error.context + ": " + error.message;
}

} // namespace vostok::graphics