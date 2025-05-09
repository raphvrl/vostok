#pragma once

#include "window/window.hpp"

#include <expected>

namespace vostok
{

class GlfwWindow : public Window
{
public:
    static std::expected<std::unique_ptr<GlfwWindow>, std::string>
    create(const WindowConfig &config = {});

    ~GlfwWindow() override;

    GlfwWindow(const GlfwWindow &) = delete;
    GlfwWindow &operator=(const GlfwWindow &) = delete;
    GlfwWindow(GlfwWindow &&) = delete;
    GlfwWindow &operator=(GlfwWindow &&) = delete;

    void pollEvents() override;
    [[nodiscard]] bool shouldClose() const override;
    void close() override;

    [[nodiscard]] u32 getWidth() const override;
    [[nodiscard]] u32 getHeight() const override;
    [[nodiscard]] f32 getAspectRatio() const override;
    [[nodiscard]] std::string getTitle() const override;

    [[nodiscard]] void *getNativeHandle() const override;
    [[nodiscard]] void *getNativeDisplay() const override;

    void setTitle(std::string_view title) override;
    void setSize(const WindowSize &size) override;
    void setVSync(bool enabled) override;
    void setFullscreen(bool enabled) override;

    using EventCallback = std::function<bool(WindowEvent event, const void *data)>;
    void setEventCallback(EventCallback callback) override;

    void centerOnScreen() override;
    void maximize() override;
    void minimize() override;
    void restore() override;
    void hide() override;
    void show() override;

    [[nodiscard]] bool isKeyPressed(KeyCode key) const override;
    [[nodiscard]] std::pair<f64, f64> getMousePosition() const override;
    void setMousePosition(f64 x, f64 y) override;
    void showCursor(bool show) override;

private:
    explicit GlfwWindow(const WindowConfig &config);

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vostok