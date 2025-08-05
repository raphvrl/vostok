

#include "vostok/core/logger/logger.hpp"
#include "vostok/graphics/buffer.hpp"
#include "vostok/graphics/gpu.hpp"
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


struct HelloTriangleApp
{
    std::unique_ptr<Window> window;
    std::unique_ptr<graphics::GPU> gpu;
    std::unique_ptr<graphics::Pipeline> pipeline;


    std::unique_ptr<graphics::Buffer> vertexBuffer;
    std::unique_ptr<graphics::Buffer> indexBuffer;
    std::unique_ptr<graphics::Buffer> uniformBuffer;
    std::unique_ptr<graphics::Buffer> storageBuffer;
    std::unique_ptr<graphics::Buffer> transferBuffer;
};


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


fs::path
findResourcePath(const fs::path &resourceType, const fs::path &resourceName)
{
    fs::path basePath = getExecutablePath();
    Logger::debug("Executable directory: {}", basePath.string());

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

    for (const auto &path : searchPaths) {
        if (fs::exists(path)) {
            Logger::debug("Found resource at: {}", path.string());
            return path;
        }
        Logger::trace("Resource not found at: {}", path.string());
    }
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
    fs::path executablePath = getExecutablePath();
    fs::path logDir = executablePath / "logs";

    try {
        if (!fs::exists(logDir)) {
            fs::create_directories(logDir);
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to create log directory: " << e.what() << "\n";
        logDir = fs::current_path() / "logs";
        try {
            if (!fs::exists(logDir)) {
                fs::create_directories(logDir);
            }
        } catch (const std::exception &e) {
            std::cerr << "Failed to create log directory in working path: "
                      << e.what() << "\n";
            logDir = fs::current_path();
        }
    }
    LogConfig logConfig;
    logConfig.level = LogLevel::DEBUG;
    logConfig.file = FileLogConfig();
    logConfig.file->filePath = (logDir / "vostok_default.log").string();
    logConfig.file->truncateOnStart = true;
    logConfig.file->separateFilesByComponent = true;
    logConfig.file->componentFilePattern =
        (logDir / "vostok_{name}.log").string();


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
std::expected<std::unique_ptr<graphics::GPU>, std::string>
createGPUDevice(Window *window)
{
    graphics::GPU::CreateInfo deviceInfo;
    deviceInfo.appName = "Vostok Hello Triangle";
    deviceInfo.appVersion = core::Version{ .major = 0, .minor = 1, .patch = 0 };
            deviceInfo.enableValidationLayers = true;
    deviceInfo.windowHandle = window->getNativeHandle();
    deviceInfo.width = window->getWidth();
    deviceInfo.height = window->getHeight();

    return graphics::GPU::create(deviceInfo);
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
createPipeline(graphics::GPU *gpu)
{

    auto pipelineBuilderResult = gpu->createPipelineBuilder();
    if (!pipelineBuilderResult) {
        return std::unexpected(pipelineBuilderResult.error());
    }

    auto pipelineBuilder = std::move(pipelineBuilderResult.value());


    fs::path vertexShaderPath =
        findResourcePath("shaders", "triangle.vert.spv");
    fs::path fragmentShaderPath =
        findResourcePath("shaders", "triangle.frag.spv");


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


    return pipelineBuilder->setName("TrianglePipeline")
        .setVertexShader(vertexShaderPath)
        .setFragmentShader(fragmentShaderPath)
        .setPrimitiveTopology(graphics::PrimitiveTopology::TRIANGLE_LIST)
        .setPolygonMode(graphics::PolygonMode::FILL)
        .setCullMode(
            graphics::CullMode::NONE
        )
        .setFrontFace(graphics::FrontFace::COUNTER_CLOCKWISE)
        .setDepthTest(false)
        .setBlend(true)
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
auto createTestBuffers(graphics::GPU *gpu) -> std::expected<
    std::tuple<
        std::unique_ptr<graphics::Buffer>,
        std::unique_ptr<graphics::Buffer>,
        std::unique_ptr<graphics::Buffer>,
        std::unique_ptr<graphics::Buffer>,
        std::unique_ptr<graphics::Buffer>>,
    std::string>
{
    Logger::info("Creating test buffers...");


    struct Vertex
    {
        float x, y, z;
        float r, g, b, a;
    };

    std::vector<Vertex> vertices = {
        { .x = -0.5F,
          .y = -0.5F,
          .z = 0.0F,
          .r = 1.0F,
          .g = 0.0F,
          .b = 0.0F,
          .a = 1.0F },
        { .x = 0.5F,
          .y = -0.5F,
          .z = 0.0F,
          .r = 0.0F,
          .g = 1.0F,
          .b = 0.0F,
          .a = 1.0F },
        { .x = 0.0F,
          .y = 0.5F,
          .z = 0.0F,
          .r = 0.0F,
          .g = 0.0F,
          .b = 1.0F,
          .a = 1.0F }
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

    struct UniformData
    {
        float time;
        std::array<f32, 2> resolution;
        std::array<f32, 62> padding;
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

    std::vector<uint8_t> transferData(1024, 0x42);

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

    Logger::info("Testing buffer operations...");
    auto mappedResult = vertexBufferResult.value()->map();
    if (mappedResult) {
        Logger::info("Successfully mapped vertex buffer");

        auto *mappedData = std::bit_cast<Vertex *>(mappedResult->data());
        mappedData[0].r = 0.5F;
        mappedData[0].g = 0.5F;

        vertexBufferResult.value()->unmap();
        Logger::info("Successfully updated vertex buffer via mapping");
    } else {
        Logger::warning(
            "Failed to map vertex buffer: {}",
            mappedResult.error()
        );
    }


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

    Logger::info("Testing buffer operators and utility methods...");
    if (*vertexBufferResult.value() == *indexBufferResult.value()) {
        Logger::info("Vertex and index buffers are equal (unexpected)");
    } else {
        Logger::info("Vertex and index buffers are different (expected)");
    }

    if (*vertexBufferResult.value() > *indexBufferResult.value()) {
        Logger::info("Vertex buffer is larger than index buffer (expected)");
    }


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


    auto fillResult = transferBufferResult.value()->fill(std::byte(0xAA));
    if (fillResult) {
        Logger::info("Successfully filled transfer buffer with 0xAA");
    } else {
        Logger::warning(
            "Failed to fill transfer buffer: {}",
            fillResult.error()
        );
    }


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


void mainLoop(HelloTriangleApp &app)
{
    Logger::info("Entering main render loop");

    uint32_t frameCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastBufferUpdateTime = startTime;

    while (!app.window->shouldClose()) {
        try {
            app.window->pollEvents();

            auto frameResult = app.gpu->beginFrame();
            if (frameResult) {
                app.pipeline->bind();
                app.gpu->draw(3, 1, 0, 0);
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


                if (frameResult.error().find("out of date") !=
                        std::string::npos ||
                    frameResult.error().find("Surface lost") !=
                        std::string::npos) {
                    Logger::info(
                        "Swapchain needs recreation, waiting for window "
                        "resize..."
                    );
    
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                } else {
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }


            auto currentTime = std::chrono::high_resolution_clock::now();
            auto timeSinceLastUpdate =
                std::chrono::duration_cast<std::chrono::seconds>(
                    currentTime - lastBufferUpdateTime
                )
                    .count();

            if (timeSinceLastUpdate >= 2) {
    
                struct UniformData
                {
                    float time;
                    std::array<f32, 2> resolution;
                    std::array<f32, 62>
                        padding;
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


            std::this_thread::sleep_for(std::chrono::milliseconds(16));


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

    if (!initializeLogger()) {
        return -1;
    }

    Logger::info("Vostok Hello Triangle starting");
    Logger::info("Working directory: {}", fs::current_path().string());

    try {
        HelloTriangleApp app;

    
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

    
        auto gpuResult = createGPUDevice(app.window.get());
        if (!gpuResult) {
            Logger::error("Failed to create GPU device: {}", gpuResult.error());
            return -1;
        }
        app.gpu = std::move(gpuResult.value());
        Logger::info("GPU device created successfully");

    
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

    
        mainLoop(app);

        Logger::info("Application exited successfully");
    } catch (const std::exception &e) {
    
        Logger::critical("Unhandled exception: {}", e.what());
        return -1;
    }


    Logger::info("Shutting down logger");
    Logger::shutdown();
    return 0;
}