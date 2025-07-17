#include "window/backends/glfw_window.hpp"

#include "core/logger/logger.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#elif defined(__linux__)
#if defined(GLFW_EXPOSE_NATIVE_X11)
#include <GLFW/glfw3native.h>
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)
#include <GLFW/glfw3native.h>
#endif
#endif

namespace vostok
{

class GlfwWindow::Impl
{
public:
    Impl(const WindowConfig &config);
    ~Impl();

    Impl(const Impl &) = delete;
    auto operator=(const Impl &) -> Impl & = delete;
    Impl(Impl &&) = delete;
    auto operator=(Impl &&) -> Impl & = delete;

    void pollEvents();
    [[nodiscard]] auto shouldClose() const -> bool;
    void close();

    [[nodiscard]] auto getWidth() const -> u32;
    [[nodiscard]] auto getHeight() const -> u32;
    [[nodiscard]] auto getAspectRatio() const -> f32;
    [[nodiscard]] auto getTitle() const -> std::string;

    [[nodiscard]] auto getNativeHandle() const -> void *;
    [[nodiscard]] auto getNativeDisplay() const -> void *;

    void setTitle(std::string_view title);
    void setSize(const WindowSize &size);
    void setVSync(bool enabled);
    void setFullscreen(bool enabled);

    void setEventCallback(Window::EventCallback callback);

    void centerOnScreen();
    void maximize();
    void minimize();
    void restore();
    void hide();
    void show();

    [[nodiscard]] auto isKeyPressed(KeyCode key) const -> bool;
    [[nodiscard]] auto getMousePosition() const -> std::pair<f64, f64>;
    void setMousePosition(f64 x, f64 y);
    void showCursor(bool show);

private:
    void setupCallbacks();

    GLFWwindow *m_window = nullptr;
    u32 m_width;
    u32 m_height;
    std::string m_title;
    bool m_vsync;
    bool m_fullscreen;

    int m_windowedPosX = 0;
    int m_windowedPosY = 0;
    u32 m_windowedWidth = 1280;
    u32 m_windowedHeight = 720;

    Window::EventCallback m_eventCallback;

    static bool g_glfwInitialized;
    static int g_windowCount;

    static auto toGlfwKey(KeyCode key) -> int;
    static auto fromGlfwKey(int key) -> KeyCode;
};

bool GlfwWindow::Impl::g_glfwInitialized = false;
int GlfwWindow::Impl::g_windowCount = 0;

GlfwWindow::Impl::Impl(const WindowConfig &config)
    : m_width(config.width),
      m_height(config.height),
      m_title(config.title),
      m_vsync(config.vsync),
      m_fullscreen(config.fullscreen)
{
    if (!g_glfwInitialized) {
        if (glfwInit() == GLFW_FALSE) {
            Logger::critical(
                "Failed to initialize GLFW - terminating application"
            );
            throw std::runtime_error("Failed to initialize GLFW");
        }
        g_glfwInitialized = true;
        glfwSetErrorCallback([](int error, const char *description) {
            Logger::error("GLFW Error [{}]: {}", error, description);
        });
        Logger::info("GLFW window system successfully initialized");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, config.decorated ? GLFW_TRUE : GLFW_FALSE);

    GLFWmonitor *monitor =
        config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    m_window = glfwCreateWindow(
        static_cast<int>(m_width),
        static_cast<int>(m_height),
        m_title.c_str(),
        monitor,
        nullptr
    );

    if (m_window == nullptr) {
        Logger::critical(
            "Window creation failed - {}x{}, title: \"{}\"",
            m_width,
            m_height,
            m_title
        );
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(m_window, this);
    setupCallbacks();
    g_windowCount++;

    Logger::info(
        "Window created: {}x{}, title: \"{}\"",
        m_width,
        m_height,
        m_title
    );
}

GlfwWindow::Impl::~Impl()
{
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        Logger::debug("Window destroyed: \"{}\"", m_title);
    }

    if (--g_windowCount <= 0) {
        glfwTerminate();
        g_glfwInitialized = false;
        Logger::info("GLFW system terminated");
    }
}

void GlfwWindow::Impl::pollEvents()
{
    if (m_window == nullptr) {
        return;
    }
    glfwPollEvents();
}

auto GlfwWindow::Impl::shouldClose() const -> bool
{
    if (m_window == nullptr) {
        return true;
    }

    return static_cast<bool>(glfwWindowShouldClose(m_window));
}

void GlfwWindow::Impl::close()
{
    if (m_window == nullptr) {
        return;
    }

    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

auto GlfwWindow::Impl::getWidth() const -> u32
{
    return m_width;
}

auto GlfwWindow::Impl::getHeight() const -> u32
{
    return m_height;
}

auto GlfwWindow::Impl::getAspectRatio() const -> f32
{
    return static_cast<f32>(m_width) / static_cast<f32>(m_height);
}

auto GlfwWindow::Impl::getTitle() const -> std::string
{
    return m_title;
}

auto GlfwWindow::Impl::getNativeHandle() const -> void *
{
    return static_cast<void *>(m_window);
}

auto GlfwWindow::Impl::getNativeDisplay() const -> void *
{
#if defined(_WIN32)
    return static_cast<void *>(GetDC(glfwGetWin32Window(m_window)));
#elif defined(__APPLE__)
    return nullptr;
#elif defined(__linux__)
#if defined(GLFW_EXPOSE_NATIVE_X11)
    return static_cast<void *>(glfwGetX11Display());
#elif defined(GLFW_EXPOSE_NATIVE_WAYLAND)
    return static_cast<void *>(glfwGetWaylandDisplay());
#else
    return nullptr;
#endif
#else
    return nullptr;
#endif
}

void GlfwWindow::Impl::setTitle(std::string_view title)
{
    if (m_window == nullptr) {
        return;
    }

    m_title = title;
    glfwSetWindowTitle(m_window, m_title.c_str());
}

void GlfwWindow::Impl::setSize(const WindowSize &size)
{
    if (m_window == nullptr) {
        return;
    }
    m_width = size.width;
    m_height = size.height;
    glfwSetWindowSize(
        m_window,
        static_cast<int>(m_width),
        static_cast<int>(m_height)
    );

    const int SIZE_DIFF_THRESHOLD = 50;
    if (std::abs(static_cast<int>(m_width) - static_cast<int>(size.width)) >
            SIZE_DIFF_THRESHOLD ||
        std::abs(static_cast<int>(m_height) - static_cast<int>(size.height)) >
            SIZE_DIFF_THRESHOLD) {
        Logger::debug(
            "Window resized: {}x{} -> {}x{}, title: \"{}\"",
            m_width,
            m_height,
            size.width,
            size.height,
            m_title
        );
    }
}

void GlfwWindow::Impl::setVSync(bool enabled)
{
    if (m_window == nullptr) {
        return;
    }

    m_vsync = enabled;
    Logger::trace(
        "VSync {} for window \"{}\"",
        enabled ? "enabled" : "disabled",
        m_title
    );
}

void GlfwWindow::Impl::setFullscreen(bool enabled)
{
    if (m_window == nullptr) {
        return;
    }

    if (m_fullscreen == enabled) {
        return;
    }

    m_fullscreen = enabled;

    if (m_fullscreen) {
        glfwGetWindowPos(m_window, &m_windowedPosX, &m_windowedPosY);
        int width = 0;
        int height = 0;
        glfwGetWindowSize(m_window, &width, &height);
        m_windowedWidth = static_cast<u32>(width);
        m_windowedHeight = static_cast<u32>(height);
        const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowMonitor(
            m_window,
            glfwGetPrimaryMonitor(),
            0,
            0,
            mode->width,
            mode->height,
            mode->refreshRate
        );
        Logger::debug(
            "Window \"{}\" switched to fullscreen mode: {}x{}",
            m_title,
            mode->width,
            mode->height
        );
    } else {
        glfwSetWindowMonitor(
            m_window,
            nullptr,
            m_windowedPosX,
            m_windowedPosY,
            static_cast<int>(m_windowedWidth),
            static_cast<int>(m_windowedHeight),
            0
        );
        Logger::debug(
            "Window \"{}\" switched to windowed mode: {}x{}",
            m_title,
            m_windowedWidth,
            m_windowedHeight
        );
    }
}

void GlfwWindow::Impl::setEventCallback(Window::EventCallback callback)
{
    m_eventCallback = std::move(callback);
}

void GlfwWindow::Impl::centerOnScreen()
{
    if (m_window == nullptr) {
        return;
    }

    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int x = (mode->width - static_cast<int>(m_width)) / 2;
    int y = (mode->height - static_cast<int>(m_height)) / 2;
    glfwSetWindowPos(m_window, x, y);
}

void GlfwWindow::Impl::maximize()
{
    if (m_window == nullptr) {
        return;
    }

    glfwMaximizeWindow(m_window);
}

void GlfwWindow::Impl::minimize()
{
    if (m_window == nullptr) {
        return;
    }

    glfwIconifyWindow(m_window);
}

void GlfwWindow::Impl::restore()
{
    if (m_window == nullptr) {
        return;
    }

    glfwRestoreWindow(m_window);
}

void GlfwWindow::Impl::hide()
{
    if (m_window == nullptr) {
        return;
    }

    glfwHideWindow(m_window);
}

void GlfwWindow::Impl::show()
{
    if (m_window == nullptr) {
        return;
    }

    glfwShowWindow(m_window);
}

auto GlfwWindow::Impl::isKeyPressed(KeyCode key) const -> bool
{
    if (m_window == nullptr) {
        return false;
    }

    int glfwKey = toGlfwKey(key);
    int state = glfwGetKey(m_window, glfwKey);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

auto GlfwWindow::Impl::getMousePosition() const -> std::pair<f64, f64>
{
    if (m_window == nullptr) {
        return { 0.0, 0.0 };
    }

    f64 x = 0.0;
    f64 y = 0.0;
    glfwGetCursorPos(m_window, &x, &y);
    return { x, y };
}

void GlfwWindow::Impl::setMousePosition(f64 x, f64 y)
{
    if (m_window == nullptr) {
        return;
    }

    glfwSetCursorPos(m_window, x, y);
}

void GlfwWindow::Impl::showCursor(bool show)
{
    if (m_window == nullptr) {
        return;
    }

    glfwSetInputMode(
        m_window,
        GLFW_CURSOR,
        show ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN
    );
    Logger::debug(
        "Cursor visibility set to {} for window \"{}\"",
        show ? "visible" : "hidden",
        m_title
    );
}

void GlfwWindow::Impl::setupCallbacks()
{
    if (m_window == nullptr) {
        return;
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    glfwSetFramebufferSizeCallback(
        m_window,
        [](GLFWwindow *window, int width, int height) {
            auto *impl = static_cast<GlfwWindow::Impl *>(
                glfwGetWindowUserPointer(window)
            );
            if (impl == nullptr || !impl->m_eventCallback) {
                return;
            }

            const u32 K_PREV_WIDTH = impl->m_width;
            const u32 K_PREV_HEIGHT = impl->m_height;

            impl->m_width = static_cast<u32>(width);
            impl->m_height = static_cast<u32>(height);

            if (std::abs(static_cast<int>(K_PREV_WIDTH) - width) > 50 ||
                std::abs(static_cast<int>(K_PREV_HEIGHT) - height) > 50) {
                Logger::trace(
                    "Framebuffer resized: {}x{} -> {}x{}",
                    K_PREV_WIDTH,
                    K_PREV_HEIGHT,
                    width,
                    height
                );
            }

            ResizeEventData data{ .width = impl->m_width,
                                  .height = impl->m_height };
            impl->m_eventCallback(WindowEvent::RESIZE, &data);
        }
    );
    glfwSetWindowCloseCallback(m_window, [](GLFWwindow *window) {
        auto *impl = static_cast<Impl *>(glfwGetWindowUserPointer(window));

        if (impl->m_eventCallback) {
            Logger::trace("Window close event received");
            impl->m_eventCallback(WindowEvent::CLOSE, nullptr);
        }
    });
    glfwSetWindowFocusCallback(m_window, [](GLFWwindow *window, int focused) {
        auto *impl = static_cast<Impl *>(glfwGetWindowUserPointer(window));

        if (impl->m_eventCallback && !focused) {
            Logger::trace("Window focus lost");
            impl->m_eventCallback(WindowEvent::UNFOCUS, nullptr);
        }
    });

    glfwSetKeyCallback(
        m_window,
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        [](GLFWwindow *window, int key, int scancode, int action, int mods) {
            auto *impl = static_cast<Impl *>(glfwGetWindowUserPointer(window));
            if (impl->m_eventCallback) {
                KeyEventData data = { .key = fromGlfwKey(key),
                                      .scancode = scancode,
                                      .pressed = action == GLFW_PRESS,
                                      .released = action == GLFW_RELEASE,
                                      .repeated = action == GLFW_REPEAT,
                                      .modifiers = static_cast<u8>(mods) };

                WindowEvent eventType = (action == GLFW_RELEASE)
                                          ? WindowEvent::KEY_RELEASE
                                          : WindowEvent::KEY_PRESS;

                if (!data.repeated) {
                    Logger::trace(
                        "Key {} event: {} (scancode: {}, mods: {})",
                        data.pressed ? "press" : "release",
                        static_cast<int>(data.key),
                        scancode,
                        mods
                    );
                }

                impl->m_eventCallback(eventType, &data);
            }
        }
    );
    glfwSetCursorPosCallback(
        m_window,
        [](GLFWwindow *window, f64 xpos, f64 ypos) {
            auto *impl = static_cast<GlfwWindow::Impl *>(
                glfwGetWindowUserPointer(window)
            );
            if (impl == nullptr || !impl->m_eventCallback) {
                return;
            }

            static int s_mouseEventCounter = 0;
            if (++s_mouseEventCounter >= 100) {
                Logger::trace("Mouse position: ({:.1f}, {:.1f})", xpos, ypos);
                s_mouseEventCounter = 0;
            }

            MouseMoveEventData data{ .x = xpos, .y = ypos };
            impl->m_eventCallback(WindowEvent::MOUSE_MOVE, &data);
        }
    );

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    glfwSetMouseButtonCallback(
        m_window,
        [](GLFWwindow *window, int button, int action, int mods) {
            auto *impl = static_cast<GlfwWindow::Impl *>(
                glfwGetWindowUserPointer(window)
            );
            if (impl == nullptr || !impl->m_eventCallback) {
                return;
            }

            f64 xpos = 0.0;
            f64 ypos = 0.0;
            glfwGetCursorPos(window, &xpos, &ypos);

            MouseButtonEventData data{ .button = button,
                                       .pressed = action == GLFW_PRESS,
                                       .modifiers = static_cast<u8>(mods),
                                       .x = xpos,
                                       .y = ypos };

            Logger::trace(
                "Mouse button {} {} at position ({:.1f}, {:.1f})",
                button,
                action == GLFW_PRESS ? "pressed" : "released",
                xpos,
                ypos
            );

            impl->m_eventCallback(WindowEvent::MOUSE_BUTTON, &data);
        }
    );
    glfwSetScrollCallback(
        m_window,
        [](GLFWwindow *window, f64 xoffset, f64 yoffset) {
            auto *impl = static_cast<GlfwWindow::Impl *>(
                glfwGetWindowUserPointer(window)
            );
            if (impl == nullptr || !impl->m_eventCallback) {
                return;
            }

            Logger::trace(
                "Mouse scroll: X: {:.2f}, Y: {:.2f}",
                xoffset,
                yoffset
            );

            MouseScrollEventData data{ .xOffset = xoffset, .yOffset = yoffset };
            impl->m_eventCallback(WindowEvent::MOUSE_SCROLL, &data);
        }
    );
}

auto GlfwWindow::Impl::toGlfwKey(KeyCode key) -> int
{
    switch (key) {
        case KeyCode::ESCAPE:
            return GLFW_KEY_ESCAPE;
        case KeyCode::ENTER:
            return GLFW_KEY_ENTER;
        case KeyCode::TAB:
            return GLFW_KEY_TAB;
        case KeyCode::BACKSPACE:
            return GLFW_KEY_BACKSPACE;
        case KeyCode::INSERT:
            return GLFW_KEY_INSERT;
        case KeyCode::DEL:
            return GLFW_KEY_DELETE;
        case KeyCode::RIGHT:
            return GLFW_KEY_RIGHT;
        case KeyCode::LEFT:
            return GLFW_KEY_LEFT;
        case KeyCode::DOWN:
            return GLFW_KEY_DOWN;
        case KeyCode::UP:
            return GLFW_KEY_UP;
        case KeyCode::PAGE_UP:
            return GLFW_KEY_PAGE_UP;
        case KeyCode::PAGE_DOWN:
            return GLFW_KEY_PAGE_DOWN;
        case KeyCode::HOME:
            return GLFW_KEY_HOME;
        case KeyCode::END:
            return GLFW_KEY_END;

        case KeyCode::SPACE:
        case KeyCode::APOSTROPHE:
        case KeyCode::COMMA:
        case KeyCode::MINUS:
        case KeyCode::PERIOD:
        case KeyCode::SLASH:
        case KeyCode::NUM_0:
        case KeyCode::NUM_1:
        case KeyCode::NUM_2:
        case KeyCode::NUM_3:
        case KeyCode::NUM_4:
        case KeyCode::NUM_5:
        case KeyCode::NUM_6:
        case KeyCode::NUM_7:
        case KeyCode::NUM_8:
        case KeyCode::NUM_9:
        case KeyCode::SEMICOLON:
        case KeyCode::EQUAL:
        case KeyCode::A:
        case KeyCode::B:
        case KeyCode::C:
        case KeyCode::D:
        case KeyCode::E:
        case KeyCode::F:
        case KeyCode::G:
        case KeyCode::H:
        case KeyCode::I:
        case KeyCode::J:
        case KeyCode::K:
        case KeyCode::L:
        case KeyCode::M:
        case KeyCode::N:
        case KeyCode::O:
        case KeyCode::P:
        case KeyCode::Q:
        case KeyCode::R:
        case KeyCode::S:
        case KeyCode::T:
        case KeyCode::U:
        case KeyCode::V:
        case KeyCode::W:
        case KeyCode::X:
        case KeyCode::Y:
        case KeyCode::Z:
        case KeyCode::LEFT_BRACKET:
        case KeyCode::BACKSLASH:
        case KeyCode::RIGHT_BRACKET:
        case KeyCode::GRAVE_ACCENT:
            return static_cast<int>(key);

        case KeyCode::UNKNOWN:
        default:
            return GLFW_KEY_UNKNOWN;
    }
}

auto GlfwWindow::Impl::fromGlfwKey(int key) -> KeyCode
{
    switch (key) {
        case GLFW_KEY_ESCAPE:
            return KeyCode::ESCAPE;
        case GLFW_KEY_ENTER:
            return KeyCode::ENTER;
        case GLFW_KEY_TAB:
            return KeyCode::TAB;
        case GLFW_KEY_BACKSPACE:
            return KeyCode::BACKSPACE;
        case GLFW_KEY_INSERT:
            return KeyCode::INSERT;
        case GLFW_KEY_DELETE:
            return KeyCode::DEL;
        case GLFW_KEY_RIGHT:
            return KeyCode::RIGHT;
        case GLFW_KEY_LEFT:
            return KeyCode::LEFT;
        case GLFW_KEY_DOWN:
            return KeyCode::DOWN;
        case GLFW_KEY_UP:
            return KeyCode::UP;
        case GLFW_KEY_PAGE_UP:
            return KeyCode::PAGE_UP;
        case GLFW_KEY_PAGE_DOWN:
            return KeyCode::PAGE_DOWN;
        case GLFW_KEY_HOME:
            return KeyCode::HOME;
        case GLFW_KEY_END:
            return KeyCode::END;

        default:
            if (key >= 32 && key <= 126) {
                return static_cast<KeyCode>(key);
            }
            break;
    }

    return KeyCode::UNKNOWN;
}

GlfwWindow::GlfwWindow(const WindowConfig &config)
    : m_impl(std::make_unique<Impl>(config))
{}

GlfwWindow::~GlfwWindow() = default;

auto GlfwWindow::create(const WindowConfig &config)
    -> std::expected<std::unique_ptr<GlfwWindow>, std::string>
{
    try {
        return std::unique_ptr<GlfwWindow>(new GlfwWindow(config));
    } catch (const std::exception &e) {
        return std::unexpected(e.what());
    } catch (...) {
        return std::unexpected(
            "Unknown error occurred while creating GLFW window"
        );
    }
}

void GlfwWindow::pollEvents()
{
    m_impl->pollEvents();
}

auto GlfwWindow::shouldClose() const -> bool
{
    return m_impl->shouldClose();
}

void GlfwWindow::close()
{
    m_impl->close();
}

auto GlfwWindow::getWidth() const -> u32
{
    return m_impl->getWidth();
}

auto GlfwWindow::getHeight() const -> u32
{
    return m_impl->getHeight();
}

auto GlfwWindow::getAspectRatio() const -> f32
{
    return m_impl->getAspectRatio();
}

auto GlfwWindow::getTitle() const -> std::string
{
    return m_impl->getTitle();
}

auto GlfwWindow::getNativeHandle() const -> void *
{
    return m_impl->getNativeHandle();
}

auto GlfwWindow::getNativeDisplay() const -> void *
{
    return m_impl->getNativeDisplay();
}

void GlfwWindow::setTitle(std::string_view title)
{
    m_impl->setTitle(title);
}

void GlfwWindow::setSize(const WindowSize &size)
{
    m_impl->setSize(size);
}

void GlfwWindow::setVSync(bool enabled)
{
    m_impl->setVSync(enabled);
}

void GlfwWindow::setFullscreen(bool enabled)
{
    m_impl->setFullscreen(enabled);
}

void GlfwWindow::setEventCallback(EventCallback callback)
{
    m_impl->setEventCallback(std::move(callback));
}

void GlfwWindow::centerOnScreen()
{
    m_impl->centerOnScreen();
}

void GlfwWindow::maximize()
{
    m_impl->maximize();
}

void GlfwWindow::minimize()
{
    m_impl->minimize();
}

void GlfwWindow::restore()
{
    m_impl->restore();
}

void GlfwWindow::hide()
{
    m_impl->hide();
}

void GlfwWindow::show()
{
    m_impl->show();
}

auto GlfwWindow::isKeyPressed(KeyCode key) const -> bool
{
    return m_impl->isKeyPressed(key);
}

auto GlfwWindow::getMousePosition() const -> std::pair<f64, f64>
{
    return m_impl->getMousePosition();
}

void GlfwWindow::setMousePosition(f64 x, f64 y)
{
    m_impl->setMousePosition(x, y);
}

void GlfwWindow::showCursor(bool show)
{
    m_impl->showCursor(show);
}

} // namespace vostok