#include "window/window.hpp"

#ifdef VOSTOK_HAS_GLFW
#include "window/backends/glfw_window.hpp"
#endif

#include "window/window_config.hpp"

namespace vostok
{

auto Window::create(const WindowConfig &config)
    -> std::expected<std::unique_ptr<Window>, std::string>
{
    WindowBackend backend = config.backend;
    if (backend == WindowBackend::AUTO) {
#ifdef VOSTOK_HAS_GLFW
        backend = WindowBackend::GLFW;
#elif defined(VOSTOK_HAS_SDL)
        backend = WindowBackend::SDL;
#else
        return std::unexpected("No supported window backend available");
#endif
    }

    switch (backend) {
        case WindowBackend::GLFW: {
#ifdef VOSTOK_HAS_GLFW
            auto result = GlfwWindow::create(config);
            if (!result) {
                return std::unexpected(result.error());
            }
            return std::unique_ptr<Window>(result.value().release());
#else
            return std::unexpected("GLFW backend is not available");
#endif
        }
        case WindowBackend::SDL: {
#ifdef VOSTOK_HAS_SDL
            return std::unexpected("SDL backend is not implemented");
#else
            return std::unexpected("SDL backend is not available");
#endif
        }

        default:
            return std::unexpected("Unknown window backend");
    }
}

} // namespace vostok
