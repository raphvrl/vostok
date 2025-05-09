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
    static std::expected<std::unique_ptr<Window>, std::string>
    create(const WindowConfig &config = {});

    virtual ~Window() = default;

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
    Window(Window &&) = delete;
    Window &operator=(Window &&) = delete;

    virtual void pollEvents() = 0;
    [[nodiscard]] virtual bool shouldClose() const = 0;
    virtual void close() = 0;

    [[nodiscard]] virtual u32 getWidth() const = 0;
    [[nodiscard]] virtual u32 getHeight() const = 0;
    [[nodiscard]] virtual f32 getAspectRatio() const = 0;
    [[nodiscard]] virtual std::string getTitle() const = 0;

    [[nodiscard]] virtual void *getNativeHandle() const = 0;
    [[nodiscard]] virtual void *getNativeDisplay() const = 0;

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

    [[nodiscard]] virtual bool isKeyPressed(KeyCode key) const = 0;
    [[nodiscard]] virtual std::pair<f64, f64> getMousePosition() const = 0;
    virtual void setMousePosition(f64 x, f64 y) = 0;
    virtual void showCursor(bool show) = 0;

protected:
    Window() = default;
};

} // namespace vostok