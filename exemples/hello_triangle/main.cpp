/**
 * @file main.cpp
 * @brief Hello Triangle Example - A simple rendering example using the Vostok
 * graphics library.
 *
 * This example demonstrates how to:
 * - Initialize a window
 * - Set up a GPU device with Vulkan
 * - Create and test different types of buffers (vertex, index, uniform,
 * storage, transfer)
 * - Test buffer operations (mapping, updating, GPU-only transfers)
 * - Create a graphics pipeline
 * - Load shaders
 * - Render a simple colored triangle
 *
 * The application uses a modular approach with robust resource management and
 * error handling. It provides comprehensive testing of the buffer system with
 * various memory types and usage patterns.
 */

#include "vostok/core/logger/logger.hpp"
#include "vostok/graphics/buffer.hpp"
#include "vostok/graphics/gpu_device.hpp"
#include "vostok/graphics/pipeline.hpp"
#include "vostok/window/window.hpp"
#include "vostok/window/window_config.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <numbers>
#include <thread>
#include <vector>

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
 * required for rendering (window, GPU device, graphics pipeline, and test
 * buffers).
 */
struct HelloTriangleApp
{
    std::unique_ptr<Window> window;
    std::unique_ptr<graphics::GPUDevice> gpu;
    std::unique_ptr<graphics::Pipeline> pipeline;

    // Test buffers
    std::unique_ptr<graphics::Buffer> vertexBuffer;
    std::unique_ptr<graphics::Buffer> indexBuffer;
    std::unique_ptr<graphics::Buffer> uniformBuffer;
    std::unique_ptr<graphics::Buffer> storageBuffer;
    std::unique_ptr<graphics::Buffer> transferBuffer;
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
fs::path
findResourcePath(const fs::path &resourceType, const fs::path &resourceName)
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
    Logger::warning(
        "Resource not found: {}/{}",
        resourceType.string(),
        resourceName.string()
    );
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
            std::cerr << "Failed to create log directory in working path: "
                      << e.what() << "\n";
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
    logConfig.file->componentFilePattern =
        (logDir / "vostok_{name}.log").string();

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
std::expected<std::unique_ptr<graphics::GPUDevice>, std::string>
createGPUDevice(Window *window)
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
    fs::path vertexShaderPath =
        findResourcePath("shaders", "triangle.vert.spv");
    fs::path fragmentShaderPath =
        findResourcePath("shaders", "triangle.frag.spv");

    // If shaders weren't found, try additional locations
    if (!fs::exists(vertexShaderPath) || !fs::exists(fragmentShaderPath)) {
        std::vector<fs::path> shaderLocations = {
            "shaders",
            "bin/shaders",
            "../bin/shaders",
            "build/bin/shaders",
            "../../../build/bin/shaders"
        };

        bool found = false;
        for (const auto &dir : shaderLocations) {
            auto vert = dir / "triangle.vert.spv";
            auto frag = dir / "triangle.frag.spv";

            if (fs::exists(vert) && fs::exists(frag)) {
                vertexShaderPath = vert;
                fragmentShaderPath = frag;
                found = true;
                Logger::info(
                    "Found shaders in fallback location: {}",
                    dir.string()
                );
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
        .setCullMode(
            graphics::CullMode::NONE
        ) // No backface culling for this simple example
        .setFrontFace(graphics::FrontFace::COUNTER_CLOCKWISE)
        .setDepthTest(false) // Disable depth testing for 2D triangle
        .setBlend(true)      // Enable blending
        .build();
}

/**
 * @brief Creates and tests different types of buffers.
 *
 * This function demonstrates the creation and usage of various buffer types:
 * - Vertex buffer (CPU accessible)
 * - Index buffer (CPU accessible)
 * - Uniform buffer (GPU only)
 * - Storage buffer (GPU only)
 * - Transfer buffer (CPU to GPU)
 *
 * @param gpu Pointer to the GPU device
 * @return A structure containing all created buffers or an error message
 */
auto createTestBuffers(graphics::GPUDevice *gpu) -> std::expected<
    std::tuple<
        std::unique_ptr<graphics::Buffer>,
        std::unique_ptr<graphics::Buffer>,
        std::unique_ptr<graphics::Buffer>,
        std::unique_ptr<graphics::Buffer>,
        std::unique_ptr<graphics::Buffer>>,
    std::string>
{
    Logger::info("Creating test buffers...");

    // 1. Vertex Buffer - CPU accessible for easy updates
    struct Vertex
    {
        float x, y, z;    // Position
        float r, g, b, a; // Color
    };

    std::vector<Vertex> vertices = {
        { .x = -0.5F,
          .y = -0.5F,
          .z = 0.0F,
          .r = 1.0F,
          .g = 0.0F,
          .b = 0.0F,
          .a = 1.0F }, // Red
        { .x = 0.5F,
          .y = -0.5F,
          .z = 0.0F,
          .r = 0.0F,
          .g = 1.0F,
          .b = 0.0F,
          .a = 1.0F }, // Green
        { .x = 0.0F,
          .y = 0.5F,
          .z = 0.0F,
          .r = 0.0F,
          .g = 0.0F,
          .b = 1.0F,
          .a = 1.0F } // Blue
    };

    graphics::BufferCreateInfo vertexBufferInfo{
        .usage = graphics::BufferUsage::VERTEX,
        .memory = graphics::BufferMemory::CPU_TO_GPU,
        .size = vertices.size() * sizeof(Vertex),
        .data = std::span<const std::byte>(
            std::bit_cast<const std::byte *>(vertices.data()),
            vertices.size() * sizeof(Vertex)
        ),
        .debugName = "TestVertexBuffer"
    };

    auto vertexBufferResult = gpu->createBuffer(vertexBufferInfo);
    if (!vertexBufferResult) {
        return std::unexpected(
            "Failed to create vertex buffer: " + vertexBufferResult.error()
        );
    }
    Logger::info(
        "Vertex buffer created successfully ({} bytes)",
        vertexBufferInfo.size
    );

    // 2. Index Buffer - CPU accessible
    std::vector<uint32_t> indices = { 0, 1, 2 };

    graphics::BufferCreateInfo indexBufferInfo{
        .usage = graphics::BufferUsage::INDEX,
        .memory = graphics::BufferMemory::CPU_TO_GPU,
        .size = indices.size() * sizeof(uint32_t),
        .data = std::span<const std::byte>(
            std::bit_cast<const std::byte *>(indices.data()),
            indices.size() * sizeof(uint32_t)
        ),
        .debugName = "TestIndexBuffer"
    };

    auto indexBufferResult = gpu->createBuffer(indexBufferInfo);
    if (!indexBufferResult) {
        return std::unexpected(
            "Failed to create index buffer: " + indexBufferResult.error()
        );
    }
    Logger::info(
        "Index buffer created successfully ({} bytes)",
        indexBufferInfo.size
    );

    // 3. Uniform Buffer - GPU only for performance
    struct UniformData
    {
        float time;
        std::array<f32, 2> resolution;
        std::array<f32, 62> padding; // Padding pour aligner sur 256 bytes (4 +
                                     // 8 + 248 = 260, aligné sur 256)
    };

    UniformData uniformData = { .time = 0.0F,
                                .resolution = { 800.0F, 600.0F },
                                .padding = { 0.0F, 0.0F } };

    graphics::BufferCreateInfo uniformBufferInfo{
        .usage = graphics::BufferUsage::UNIFORM |
                 graphics::BufferUsage::TRANSFER_DST,
        .memory = graphics::BufferMemory::GPU_ONLY,
        .size = sizeof(UniformData),
        .data = std::span<const std::byte>(
            std::bit_cast<const std::byte *>(&uniformData),
            sizeof(UniformData)
        ),
        .debugName = "TestUniformBuffer"
    };

    auto uniformBufferResult = gpu->createBuffer(uniformBufferInfo);
    if (!uniformBufferResult) {
        return std::unexpected(
            "Failed to create uniform buffer: " + uniformBufferResult.error()
        );
    }
    Logger::info(
        "Uniform buffer created successfully ({} bytes)",
        uniformBufferInfo.size
    );

    // 4. Storage Buffer - GPU only
    std::vector<float> storageData = { 1.0F, 2.0F, 3.0F, 4.0F,
                                       5.0F, 6.0F, 7.0F, 8.0F };

    graphics::BufferCreateInfo storageBufferInfo{
        .usage = graphics::BufferUsage::STORAGE |
                 graphics::BufferUsage::TRANSFER_DST,
        .memory = graphics::BufferMemory::GPU_ONLY,
        .size = storageData.size() * sizeof(float),
        .data = std::span<const std::byte>(
            std::bit_cast<const std::byte *>(storageData.data()),
            storageData.size() * sizeof(float)
        ),
        .debugName = "TestStorageBuffer"
    };

    auto storageBufferResult = gpu->createBuffer(storageBufferInfo);
    if (!storageBufferResult) {
        return std::unexpected(
            "Failed to create storage buffer: " + storageBufferResult.error()
        );
    }
    Logger::info(
        "Storage buffer created successfully ({} bytes)",
        storageBufferInfo.size
    );

    // 5. Transfer Buffer - CPU to GPU for staging
    std::vector<uint8_t> transferData(1024, 0x42); // 1KB of test data

    graphics::BufferCreateInfo transferBufferInfo{
        .usage = graphics::BufferUsage::TRANSFER_SRC,
        .memory = graphics::BufferMemory::CPU_TO_GPU,
        .size = transferData.size(),
        .data = std::as_bytes(std::span(transferData)),
        .debugName = "TestTransferBuffer"
    };

    auto transferBufferResult = gpu->createBuffer(transferBufferInfo);
    if (!transferBufferResult) {
        return std::unexpected(
            "Failed to create transfer buffer: " + transferBufferResult.error()
        );
    }
    Logger::info(
        "Transfer buffer created successfully ({} bytes)",
        transferBufferInfo.size
    );

    // Test buffer operations
    Logger::info("Testing buffer operations...");

    // Test mapping and updating CPU accessible buffer
    auto mappedResult = vertexBufferResult.value()->map();
    if (mappedResult) {
        Logger::info("Successfully mapped vertex buffer");

        // Modify the first vertex color
        auto *mappedData = std::bit_cast<Vertex *>(mappedResult->data());
        mappedData[0].r = 0.5F; // Change red to 0.5
        mappedData[0].g = 0.5F; // Change green to 0.5

        vertexBufferResult.value()->unmap();
        Logger::info("Successfully updated vertex buffer via mapping");
    } else {
        Logger::warning(
            "Failed to map vertex buffer: {}",
            mappedResult.error()
        );
    }

    // Test updating GPU only buffer
    UniformData newUniformData = { .time = 1.0F,
                                   .resolution = { 1024.0F, 768.0F },
                                   .padding = { 0.0F, 0.0F } };

    auto updateResult = uniformBufferResult.value()->update(
        std::span<const std::byte>(
            std::bit_cast<const std::byte *>(&newUniformData),
            sizeof(UniformData)
        )
    );

    if (updateResult) {
        Logger::info("Successfully updated GPU-only uniform buffer");
    } else {
        Logger::warning(
            "Failed to update uniform buffer: {}",
            updateResult.error()
        );
    }

    // Test buffer properties
    Logger::info("Buffer properties:");
    Logger::info(
        "  Vertex buffer: size={}, usage={}, memory={}",
        vertexBufferResult.value()->getSize(),
        static_cast<int>(vertexBufferResult.value()->getUsage()),
        static_cast<int>(vertexBufferResult.value()->getMemory())
    );
    Logger::info(
        "  Index buffer: size={}, usage={}, memory={}",
        indexBufferResult.value()->getSize(),
        static_cast<int>(indexBufferResult.value()->getUsage()),
        static_cast<int>(indexBufferResult.value()->getMemory())
    );
    Logger::info(
        "  Uniform buffer: size={}, usage={}, memory={}",
        uniformBufferResult.value()->getSize(),
        static_cast<int>(uniformBufferResult.value()->getUsage()),
        static_cast<int>(uniformBufferResult.value()->getMemory())
    );

    Logger::info("All test buffers created and tested successfully!");

    // Test des nouveaux opérateurs et méthodes utilitaires
    Logger::info("Testing buffer operators and utility methods...");

    // Test des opérateurs de comparaison
    if (*vertexBufferResult.value() == *indexBufferResult.value()) {
        Logger::info("Vertex and index buffers are equal (unexpected)");
    } else {
        Logger::info("Vertex and index buffers are different (expected)");
    }

    if (*vertexBufferResult.value() > *indexBufferResult.value()) {
        Logger::info("Vertex buffer is larger than index buffer (expected)");
    }

    // Test des méthodes utilitaires
    if (!vertexBufferResult.value()->isEmpty()) {
        Logger::info("Vertex buffer is not empty (expected)");
    }

    Logger::info(
        "Vertex buffer available size: {} bytes",
        vertexBufferResult.value()->getAvailableSize()
    );

    if (vertexBufferResult.value()->isValidOffset(0)) {
        Logger::info("Offset 0 is valid for vertex buffer");
    }

    if (vertexBufferResult.value()->isValidRange(0, 84)) {
        Logger::info("Range [0, 84] is valid for vertex buffer");
    }

    // Test de la méthode fill
    auto fillResult = transferBufferResult.value()->fill(std::byte(0xAA));
    if (fillResult) {
        Logger::info("Successfully filled transfer buffer with 0xAA");
    } else {
        Logger::warning(
            "Failed to fill transfer buffer: {}",
            fillResult.error()
        );
    }

    // Test de la méthode updateData avec template
    float testFloat = std::numbers::pi_v<float>;
    auto updateFloatResult = uniformBufferResult.value()->updateData(testFloat);
    if (updateFloatResult) {
        Logger::info("Successfully updated uniform buffer with float value");
    } else {
        Logger::warning(
            "Failed to update uniform buffer with float: {}",
            updateFloatResult.error()
        );
    }

    // Test de la méthode updateArray avec template
    std::vector<int> testArray = { 1, 2, 3, 4, 5 };
    auto updateArrayResult =
        storageBufferResult.value()->updateArray(testArray);
    if (updateArrayResult) {
        Logger::info("Successfully updated storage buffer with int array");
    } else {
        Logger::warning(
            "Failed to update storage buffer with array: {}",
            updateArrayResult.error()
        );
    }

    Logger::info("Buffer operators and utility methods tested successfully!");

    return std::make_tuple(
        std::move(vertexBufferResult.value()),
        std::move(indexBufferResult.value()),
        std::move(uniformBufferResult.value()),
        std::move(storageBufferResult.value()),
        std::move(transferBufferResult.value())
    );
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
    auto lastBufferUpdateTime = startTime;

    // Main loop
    while (!app.window->shouldClose()) {
        try {
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
                    Logger::warning(
                        "Failed to end frame: {}",
                        endResult.error()
                    );
                }

                frameCount++;
            } else {
                Logger::warning(
                    "Failed to begin frame: {}",
                    frameResult.error()
                );

                // Check if the error is related to swapchain being out of date
                if (frameResult.error().find("out of date") !=
                        std::string::npos ||
                    frameResult.error().find("Surface lost") !=
                        std::string::npos) {
                    Logger::info(
                        "Swapchain needs recreation, waiting for window "
                        "resize..."
                    );
                    // Wait longer for window resize events
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                } else {
                    // Add a small delay when frame acquisition fails to prevent
                    // tight loop
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            // Update buffer data every 2 seconds to test dynamic updates
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto timeSinceLastUpdate =
                std::chrono::duration_cast<std::chrono::seconds>(
                    currentTime - lastBufferUpdateTime
                )
                    .count();

            if (timeSinceLastUpdate >= 2) {
                // Test updating the uniform buffer with current time
                struct UniformData
                {
                    float time;
                    std::array<f32, 2> resolution;
                    std::array<f32, 62>
                        padding; // Padding pour aligner sur 256 bytes
                };

                auto currentTimeValue = static_cast<float>(timeSinceLastUpdate);
                UniformData newUniformData = {
                    .time = currentTimeValue,
                    .resolution = { static_cast<float>(app.window->getWidth()),
                                    static_cast<float>(
                                        app.window->getHeight()
                                    ) },
                    .padding = { 0.0F, 0.0F }
                };

                auto updateResult = app.uniformBuffer->update(
                    std::span<const std::byte>(
                        std::bit_cast<const std::byte *>(&newUniformData),
                        sizeof(UniformData)
                    )
                );

                if (updateResult) {
                    Logger::debug(
                        "Updated uniform buffer with time: {:.2f}",
                        currentTimeValue
                    );
                } else {
                    Logger::warning(
                        "Failed to update uniform buffer: {}",
                        updateResult.error()
                    );
                }

                lastBufferUpdateTime = currentTime;
            }

            // Limit frame rate to approximately 60 FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(16));

            // Calculate and display FPS every 5 seconds
            auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(
                                   currentTime - startTime
            )
                                   .count();

            if (elapsedTime >= 5) {
                float fps = static_cast<float>(frameCount) /
                            static_cast<float>(elapsedTime);
                Logger::info("Average FPS: {:.2f}", fps);
                Logger::info(
                    "Buffer status: vertex={} bytes, index={} bytes, "
                    "uniform={} "
                    "bytes, storage={} "
                    "bytes, transfer={} bytes",
                    app.vertexBuffer->getSize(),
                    app.indexBuffer->getSize(),
                    app.uniformBuffer->getSize(),
                    app.storageBuffer->getSize(),
                    app.transferBuffer->getSize()
                );
                frameCount = 0;
                startTime = currentTime;
            }
        } catch (const std::exception &e) {
            Logger::critical("Exception in main loop: {}", e.what());
            break;
        } catch (...) {
            Logger::critical("Unknown exception in main loop");
            break;
        }
    }

    // Wait for the GPU to finish all operations before cleanup
    try {
        app.gpu->waitIdle();
        Logger::info("Exiting main render loop");
    } catch (const std::exception &e) {
        Logger::critical("Exception during GPU wait: {}", e.what());
    }
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
auto main(int argc, char *argv[]) -> int
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
            Logger::error(
                "Failed to create pipeline: {}",
                pipelineResult.error()
            );
            return -1;
        }
        app.pipeline = std::move(pipelineResult.value());
        Logger::info("Pipeline created successfully");

        // Create and test buffers
        auto buffersResult = createTestBuffers(app.gpu.get());
        if (!buffersResult) {
            Logger::error(
                "Failed to create test buffers: {}",
                buffersResult.error()
            );
            return -1;
        }
        app.vertexBuffer = std::move(std::get<0>(buffersResult.value()));
        app.indexBuffer = std::move(std::get<1>(buffersResult.value()));
        app.uniformBuffer = std::move(std::get<2>(buffersResult.value()));
        app.storageBuffer = std::move(std::get<3>(buffersResult.value()));
        app.transferBuffer = std::move(std::get<4>(buffersResult.value()));
        Logger::info("Test buffers created and assigned to app");

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