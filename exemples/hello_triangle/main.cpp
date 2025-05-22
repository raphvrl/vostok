/**
 * @file main.cpp
 * @brief Hello Triangle Example - A simple rendering example using the Vostok graphics library.
 *
 * This example demonstrates how to:
 * - Initialize a window
 * - Set up a GPU device with Vulkan
 * - Create a graphics pipeline
 * - Load shaders
 * - Render a simple colored triangle
 *
 * The application uses a modular approach with robust resource management and error handling.
 */

#include "vostok/core/logger/logger.hpp"
#include "vostok/graphics/gpu_device.hpp"
#include "vostok/graphics/pipeline.hpp"
#include "vostok/window/window.hpp"
#include "vostok/window/window_config.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;
using namespace vostok;

/**
 * @brief Container for all major application resources.
 *
 * This structure centralizes the ownership of the main components
 * required for rendering (window, GPU device, and graphics pipeline).
 */
struct HelloTriangleApp
{
    std::unique_ptr<Window> window;
    std::unique_ptr<graphics::GPUDevice> gpu;
    std::unique_ptr<graphics::Pipeline> pipeline;
};

/**
 * @brief Gets the directory path of the current executable.
 *
 * This function is platform-specific and handles the differences
 * between Windows and Linux/Unix systems.
 *
 * @return The path to the directory containing the executable.
 */
fs::path getExecutablePath()
{
#ifdef _WIN32
    std::array<wchar_t, MAX_PATH> path{};
    GetModuleFileNameW(nullptr, static_cast<LPWSTR>(path.data()), MAX_PATH);
    return fs::path(path.data()).parent_path();
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return fs::path(std::string(result, (count > 0) ? count : 0)).parent_path();
#endif
}

/**
 * @brief Locates a resource file using multiple search paths.
 *
 * This function implements a robust resource location strategy by searching
 * for files in multiple common locations relative to:
 * - The executable directory
 * - The parent directory
 * - The current working directory
 *
 * @param resourceType Type of resource (e.g., "shaders", "textures")
 * @param resourceName Name of the resource file
 * @return Path to the resource if found, or a default path if not found
 */
fs::path findResourcePath(const fs::path &resourceType, const fs::path &resourceName)
{
    // Base path is the executable's directory
    fs::path basePath = getExecutablePath();
    Logger::debug("Executable directory: {}", basePath.string());

    // List of possible paths to search for the resource
    std::vector<fs::path> searchPaths = {
        basePath / resourceType / resourceName,
        basePath / "resources" / resourceType / resourceName,
        basePath.parent_path() / resourceType / resourceName,
        basePath.parent_path() / "resources" / resourceType / resourceName,
        basePath.parent_path() / "bin" / resourceType / resourceName,
        fs::current_path() / resourceType / resourceName,
        fs::current_path() / "resources" / resourceType / resourceName,
        fs::current_path() / "bin" / resourceType / resourceName
    };

    // Return the first valid path found
    for (const auto &path : searchPaths) {
        if (fs::exists(path)) {
            Logger::debug("Found resource at: {}", path.string());
            return path;
        }
        Logger::trace("Resource not found at: {}", path.string());
    }

    // If no path is found, return a default path and log a warning
    Logger::warning("Resource not found: {}/{}", resourceType.string(), resourceName.string());
    return basePath / resourceType / resourceName;
}

/**
 * @brief Initializes the logging system with file output.
 *
 * Sets up the logger with appropriate configuration and creates a dedicated
 * logs directory. Includes fallback mechanisms if the primary log directory
 * cannot be created.
 *
 * @return true if logger was initialized successfully, false otherwise
 */
bool initializeLogger()
{
    // Try to create a logs directory next to the executable
    fs::path executablePath = getExecutablePath();
    fs::path logDir = executablePath / "logs";

    try {
        if (!fs::exists(logDir)) {
            fs::create_directories(logDir);
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to create log directory: " << e.what() << "\n";
        // Fall back to the current working directory
        logDir = fs::current_path() / "logs";
        try {
            if (!fs::exists(logDir)) {
                fs::create_directories(logDir);
            }
        } catch (const std::exception &e) {
            std::cerr << "Failed to create log directory in working path: " << e.what() << "\n";
            // If both fail, use the current directory
            logDir = fs::current_path();
        }
    }

    // Configure the logger
    LogConfig logConfig;
    logConfig.level = LogLevel::DEBUG;
    logConfig.file = FileLogConfig();
    logConfig.file->filePath = (logDir / "vostok_default.log").string();
    logConfig.file->truncateOnStart = true;
    logConfig.file->separateFilesByComponent = true;
    logConfig.file->componentFilePattern = (logDir / "vostok_{name}.log").string();

    // Initialize the logger
    auto result = Logger::init(logConfig);
    if (!result) {
        std::cerr << "Failed to initialize logger: " << result.error() << "\n";
        return false;
    }

    Logger::info("Logger initialized");
    Logger::info("Logs will be stored in: {}", logDir.string());
    return true;
}

/**
 * @brief Creates and configures the application window.
 *
 * @return A window instance or an error message if creation failed
 */
std::expected<std::unique_ptr<Window>, std::string> createWindow()
{
    WindowConfig windowInfo;
    windowInfo.title = "Vostok Hello Triangle";
    windowInfo.width = 800;
    windowInfo.height = 600;
    windowInfo.resizable = true;

    return Window::create(windowInfo);
}

/**
 * @brief Creates and initializes the GPU device.
 *
 * This function sets up the Vulkan device with the application information
 * and connects it to the window surface for rendering.
 *
 * @param window Pointer to the window to render to
 * @return A GPU device instance or an error message if creation failed
 */
std::expected<std::unique_ptr<graphics::GPUDevice>, std::string> createGPUDevice(Window *window)
{
    graphics::GPUDevice::CreateInfo deviceInfo;
    deviceInfo.appName = "Vostok Hello Triangle";
    deviceInfo.appVersion = core::Version{ .major = 0, .minor = 1, .patch = 0 };
    deviceInfo.enableValidationLayers = true; // Enable validation for debugging
    deviceInfo.windowHandle = window->getNativeHandle();
    deviceInfo.width = window->getWidth();
    deviceInfo.height = window->getHeight();

    return graphics::GPUDevice::create(deviceInfo);
}

/**
 * @brief Creates a graphics pipeline for rendering the triangle.
 *
 * This function:
 * 1. Creates a pipeline builder
 * 2. Locates the necessary shader files
 * 3. Configures the pipeline state
 * 4. Builds and returns the pipeline
 *
 * @param gpu Pointer to the GPU device
 * @return A pipeline instance or an error message if creation failed
 */
std::expected<std::unique_ptr<graphics::Pipeline>, std::string>
createPipeline(graphics::GPUDevice *gpu)
{
    // Get a pipeline builder from the GPU device
    auto pipelineBuilderResult = gpu->createPipelineBuilder();
    if (!pipelineBuilderResult) {
        return std::unexpected(pipelineBuilderResult.error());
    }

    auto pipelineBuilder = std::move(pipelineBuilderResult.value());

    // Locate shader files
    fs::path vertexShaderPath = findResourcePath("shaders", "triangle.vert.spv");
    fs::path fragmentShaderPath = findResourcePath("shaders", "triangle.frag.spv");

    // If shaders weren't found, try additional locations
    if (!fs::exists(vertexShaderPath) || !fs::exists(fragmentShaderPath)) {
        std::vector<fs::path> shaderLocations = { "shaders",
                                                  "bin/shaders",
                                                  "../bin/shaders",
                                                  "build/bin/shaders",
                                                  "../../../build/bin/shaders" };

        bool found = false;
        for (const auto &dir : shaderLocations) {
            auto vert = dir / "triangle.vert.spv";
            auto frag = dir / "triangle.frag.spv";

            if (fs::exists(vert) && fs::exists(frag)) {
                vertexShaderPath = vert;
                fragmentShaderPath = frag;
                found = true;
                Logger::info("Found shaders in fallback location: {}", dir.string());
                break;
            }
        }

        if (!found) {
            return std::unexpected("Could not find shader files");
        }
    } else {
        Logger::info("Found shaders:");
        Logger::info("  Vertex shader: {}", vertexShaderPath.string());
        Logger::info("  Fragment shader: {}", fragmentShaderPath.string());
    }

    // Configure and build the pipeline
    return pipelineBuilder->setName("TrianglePipeline")
        .setVertexShader(vertexShaderPath)
        .setFragmentShader(fragmentShaderPath)
        .setPrimitiveTopology(graphics::PrimitiveTopology::TRIANGLE_LIST)
        .setPolygonMode(graphics::PolygonMode::FILL)
        .setCullMode(graphics::CullMode::NONE) // No backface culling for this simple example
        .setFrontFace(graphics::FrontFace::COUNTER_CLOCKWISE)
        .setDepthTest(false) // Disable depth testing for 2D triangle
        .setBlend(true)      // Enable blending
        .build();
}

/**
 * @brief Main rendering loop.
 *
 * This function handles:
 * - Window event processing
 * - Frame rendering
 * - Performance monitoring
 * - Application exit
 *
 * @param app Reference to the application resources
 */
void mainLoop(HelloTriangleApp &app)
{
    Logger::info("Entering main render loop");

    // Performance monitoring
    uint32_t frameCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();

    // Main loop
    while (!app.window->shouldClose()) {
        // Process window events (keyboard, mouse, etc.)
        app.window->pollEvents();

        // Begin a new frame
        auto frameResult = app.gpu->beginFrame();
        if (frameResult) {
            // Bind the pipeline and draw the triangle
            app.pipeline->bind();
            app.gpu->draw(3, 1, 0, 0); // 3 vertices = 1 triangle

            // End the frame and present it
            auto endResult = app.gpu->endFrame();
            if (!endResult) {
                Logger::warning("Failed to end frame: {}", endResult.error());
            }

            frameCount++;
        }

        // Limit frame rate to approximately 60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        // Calculate and display FPS every 5 seconds
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime =
            std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

        if (elapsedTime >= 5) {
            float fps = static_cast<float>(frameCount) / static_cast<float>(elapsedTime);
            Logger::info("Average FPS: {:.2f}", fps);
            frameCount = 0;
            startTime = currentTime;
        }
    }

    // Wait for the GPU to finish all operations before cleanup
    app.gpu->waitIdle();
    Logger::info("Exiting main render loop");
}

/**
 * @brief Application entry point.
 *
 * This function orchestrates the entire application:
 * 1. Initializes systems
 * 2. Creates resources
 * 3. Runs the main loop
 * 4. Handles cleanup and shutdown
 *
 * @param argc Argument count (unused)
 * @param argv Argument values (unused)
 * @return 0 on success, -1 on failure
 */
int main(int argc, char *argv[])
{
    // Initialize the logging system
    if (!initializeLogger()) {
        return -1;
    }

    Logger::info("Vostok Hello Triangle starting");
    Logger::info("Working directory: {}", fs::current_path().string());

    try {
        HelloTriangleApp app;

        // Create and initialize the window
        auto windowResult = createWindow();
        if (!windowResult) {
            Logger::error("Failed to create window: {}", windowResult.error());
            return -1;
        }
        app.window = std::move(windowResult.value());
        Logger::info(
            "Window created successfully: {}x{}",
            app.window->getWidth(),
            app.window->getHeight()
        );

        // Create and initialize the GPU device
        auto gpuResult = createGPUDevice(app.window.get());
        if (!gpuResult) {
            Logger::error("Failed to create GPU device: {}", gpuResult.error());
            return -1;
        }
        app.gpu = std::move(gpuResult.value());
        Logger::info("GPU device created successfully");

        // Create the rendering pipeline
        auto pipelineResult = createPipeline(app.gpu.get());
        if (!pipelineResult) {
            Logger::error("Failed to create pipeline: {}", pipelineResult.error());
            return -1;
        }
        app.pipeline = std::move(pipelineResult.value());
        Logger::info("Pipeline created successfully");

        // Run the main application loop
        mainLoop(app);

        Logger::info("Application exited successfully");
    } catch (const std::exception &e) {
        // Catch and log any unhandled exceptions
        Logger::critical("Unhandled exception: {}", e.what());
        return -1;
    }

    // Clean shutdown
    Logger::info("Shutting down logger");
    Logger::shutdown();
    return 0;
}