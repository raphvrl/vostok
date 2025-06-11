#pragma once

#include "window/window.hpp"

#include <expected>

namespace vostok
{

class GlfwWindow : public Window
{
public:
    static auto create(const WindowConfig &config = {})
        -> std::expected<std::unique_ptr<GlfwWindow>, std::string>;

    ~GlfwWindow() override;

    GlfwWindow(const GlfwWindow &) = delete;
    auto operator=(const GlfwWindow &) -> GlfwWindow & = delete;
    GlfwWindow(GlfwWindow &&) = delete;
    auto operator=(GlfwWindow &&) -> GlfwWindow & = delete;

    void pollEvents() override;
    [[nodiscard]] auto shouldClose() const -> bool override;
    void close() override;

    [[nodiscard]] auto getWidth() const -> u32 override;
    [[nodiscard]] auto getHeight() const -> u32 override;
    [[nodiscard]] auto getAspectRatio() const -> f32 override;
    [[nodiscard]] auto getTitle() const -> std::string override;

    [[nodiscard]] auto getNativeHandle() const -> void * override;
    [[nodiscard]] auto getNativeDisplay() const -> void * override;

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

    [[nodiscard]] auto isKeyPressed(KeyCode key) const -> bool override;
    [[nodiscard]] auto getMousePosition() const -> std::pair<f64, f64> override;
    void setMousePosition(f64 x, f64 y) override;
    void showCursor(bool show) override;

private:
    explicit GlfwWindow(const WindowConfig &config);

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vostok