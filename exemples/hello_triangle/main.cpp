#include "vostok/core/logger/logger.hpp"
#include "vostok/graphics/backends/vulkan/vulkan_gpu.hpp"
#include "vostok/graphics/backends/vulkan/vulkan_pipeline.hpp"
#include "vostok/graphics/buffers/bindable_resource_ptr.hpp"
#include "vostok/graphics/gpu.hpp"
#include "vostok/graphics/pipeline.hpp"
#include "vostok/math/types.hpp"
#include "vostok/window/window.hpp"
#include "vostok/window/window_config.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
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

struct ColorUBO
{
    alignas(16) math::Vec3 color;
};

struct HelloTriangleApp
{
    std::unique_ptr<Window> window;
    std::unique_ptr<graphics::GPU> gpu;
    std::unique_ptr<graphics::Pipeline> pipeline;

    graphics::UBOPtr<ColorUBO> colorUBO;
};

auto getExecutablePath() -> fs::path
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

auto findResourcePath(
    const fs::path &resourceType,
    const fs::path &resourceName
) -> fs::path
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

auto initializeLogger() -> bool
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
    logConfig.asyncMode = false;
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

auto createWindow() -> std::expected<std::unique_ptr<Window>, std::string>
{
    WindowConfig windowInfo;
    windowInfo.title = "Vostok Hello Triangle";
    windowInfo.width = 800;
    windowInfo.height = 600;
    windowInfo.resizable = true;

    return Window::create(windowInfo);
}

auto createGPUDevice(Window *window)
    -> std::expected<std::unique_ptr<graphics::GPU>, std::string>
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

auto createPipeline(graphics::GPU *gpu)
    -> std::expected<std::unique_ptr<graphics::Pipeline>, std::string>
{
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

    graphics::PipelineCreateInfo pipelineInfo;
    pipelineInfo.name = "TrianglePipeline";
    pipelineInfo.vertexShader = vertexShaderPath;
    pipelineInfo.fragmentShader = fragmentShaderPath;
    pipelineInfo.primitiveTopology = graphics::PrimitiveTopology::TRIANGLE_LIST;
    pipelineInfo.polygonMode = graphics::PolygonMode::FILL;
    pipelineInfo.cullMode = graphics::CullMode::NONE;
    pipelineInfo.frontFace = graphics::FrontFace::COUNTER_CLOCKWISE;
    pipelineInfo.lineWidth = 1.0F;

    pipelineInfo.depthTest = false;
    pipelineInfo.depthWrite = false;
    pipelineInfo.depthCompareOp = graphics::CompareOp::LESS;
    pipelineInfo.stencilTest = false;

    pipelineInfo.blend = true;
    pipelineInfo.srcColorBlendFactor = graphics::BlendFactor::SRC_ALPHA;
    pipelineInfo.dstColorBlendFactor =
        graphics::BlendFactor::ONE_MINUS_SRC_ALPHA;
    pipelineInfo.srcAlphaBlendFactor = graphics::BlendFactor::ONE;
    pipelineInfo.dstAlphaBlendFactor = graphics::BlendFactor::ZERO;
    pipelineInfo.colorBlendOp = graphics::BlendOp::ADD;
    pipelineInfo.alphaBlendOp = graphics::BlendOp::ADD;
    pipelineInfo.colorWriteMask = graphics::ColorComponentFlags::ALL;

    pipelineInfo.pushConstantSize = sizeof(math::Vec3);

    auto *vulkanGPU = dynamic_cast<vostok::graphics::vulkan::VulkanGPU *>(gpu);
    auto pipelineResult = vostok::graphics::vulkan::VulkanPipeline::create(
        vulkanGPU,
        pipelineInfo
    );

    if (!pipelineResult) {
        return std::unexpected(
            "Failed to create pipeline: " + pipelineResult.error()
        );
    }

    Logger::info("Pipeline created successfully with new system");
    return std::move(pipelineResult.value());
}

auto createUBO(graphics::GPU *gpu)
    -> std::expected<std::unique_ptr<graphics::UBO<ColorUBO>>, std::string>
{
    ColorUBO initialData = { .color = math::Vec3{ 1.0F, 0.0F, 0.0F } };

    return gpu->createUBO<ColorUBO>(initialData);
}

auto mainLoop(HelloTriangleApp &app) -> void
{
    Logger::info("Entering main render loop");

    uint32_t frameCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!app.window->shouldClose()) {
        try {
            app.window->pollEvents();

            auto frameResult = app.gpu->beginFrame();
            if (frameResult) {
                app.pipeline->bind();

                const f32 T = static_cast<f32>(frameCount) * 0.02F;
                if (app.colorUBO) {
                    app.colorUBO->color.x = 0.5F + 0.5F * std::sin(T);
                    app.colorUBO->color.y =
                        0.5F + 0.5F * std::sin(T + 2.0943951F);
                    app.colorUBO->color.z =
                        0.5F + 0.5F * std::sin(T + 4.1887902F);
                }

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

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
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

        auto uboResult = createUBO(app.gpu.get());
        if (!uboResult) {
            Logger::error("Failed to create UBO: {}", uboResult.error());
            return -1;
        }
        app.colorUBO = graphics::UBOPtr<ColorUBO>(std::move(uboResult.value()));
        Logger::info("UBO created successfully");

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