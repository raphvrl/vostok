#pragma once

#include "window_config.hpp"
#include "window_events.hpp"

#include <expected>
#include <functional>
#include <memory>

namespace vostok
{

struct WindowSize
{
    u32 width;
    u32 height;
};

class Window
{
public:
    static auto create(const WindowConfig &config = {})
        -> std::expected<std::unique_ptr<Window>, std::string>;

    virtual ~Window() = default;

    Window(const Window &) = delete;
    auto operator=(const Window &) -> Window & = delete;
    Window(Window &&) = delete;
    auto operator=(Window &&) -> Window & = delete;

    virtual void pollEvents() = 0;
    [[nodiscard]] virtual auto shouldClose() const -> bool = 0;
    virtual void close() = 0;

    [[nodiscard]] virtual auto getWidth() const -> u32 = 0;
    [[nodiscard]] virtual auto getHeight() const -> u32 = 0;
    [[nodiscard]] virtual auto getAspectRatio() const -> f32 = 0;
    [[nodiscard]] virtual auto getTitle() const -> std::string = 0;

    [[nodiscard]] virtual auto getNativeHandle() const -> void * = 0;
    [[nodiscard]] virtual auto getNativeDisplay() const -> void * = 0;

    virtual void setTitle(std::string_view title) = 0;
    virtual void setSize(const WindowSize &size) = 0;
    virtual void setVSync(bool enabled) = 0;
    virtual void setFullscreen(bool enabled) = 0;

    using EventCallback = std::function<bool(WindowEvent event, const void *data)>;
    virtual void setEventCallback(EventCallback callback) = 0;

    virtual void centerOnScreen() = 0;
    virtual void maximize() = 0;
    virtual void minimize() = 0;
    virtual void restore() = 0;
    virtual void hide() = 0;
    virtual void show() = 0;

    [[nodiscard]] virtual auto isKeyPressed(KeyCode key) const -> bool = 0;
    [[nodiscard]] virtual auto getMousePosition() const -> std::pair<f64, f64> = 0;
    virtual void setMousePosition(f64 x, f64 y) = 0;
    virtual void showCursor(bool show) = 0;

protected:
    Window() = default;
};

} // namespace vostok