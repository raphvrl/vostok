#pragma once

#include "vostok/core/type.hpp"

#include <string>

namespace vostok
{

enum class WindowBackend : u8
{
    AUTO,
    GLFW,
    SDL,
};

struct WindowConfig
{
    std::string title = "Vostok Engine";
    u32 width = 1280;
    u32 height = 720;
    bool resizable = true;
    bool fullscreen = false;
    bool vsync = true;
    bool decorated = true;
    WindowBackend backend = WindowBackend::AUTO;
};

} // namespace vostok