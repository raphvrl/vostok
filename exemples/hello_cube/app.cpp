#include "app.hpp"

#include "vostok/graphics/buffers/texture_loader.hpp"

#include <array>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

App::App() = default;

App::~App() = default;

auto App::initialize() -> bool
{
    Logger::info("Initializing App...");

    if (!createWindow()) {
        return false;
    }
    if (!createGPUDevice()) {
        return false;
    }
    if (!createMesh()) {
        return false;
    }
    if (!createTexture()) {
        return false;
    }
    if (!createPipeline()) {
        return false;
    }
    if (!createUBO()) {
        return false;
    }

    m_startTime = std::chrono::high_resolution_clock::now();
    m_isRunning = true;

    Logger::info("App initialized successfully");
    return true;
}

auto App::run() -> void
{
    Logger::info("Starting main loop...");
    mainLoop();
}

auto App::shutdown() -> void
{
    if (m_gpu) {
        try {
            m_gpu->waitIdle();
        } catch (const std::exception &e) {
            Logger::warning("Exception during GPU wait: {}", e.what());
        }
    }

    m_isRunning = false;
    Logger::info("App shutdown complete");
}

auto App::createWindow() -> bool
{
    WindowConfig windowInfo;
    windowInfo.title = "Hello cube !";
    windowInfo.width = 800;
    windowInfo.height = 600;
    windowInfo.resizable = true;

    auto windowResult = Window::create(windowInfo);
    if (!windowResult) {
        Logger::error("Failed to create window: {}", windowResult.error());
        return false;
    }

    m_window = std::move(windowResult.value());
    Logger::info(
        "Window created: {}x{}",
        m_window->getWidth(),
        m_window->getHeight()
    );

    m_window->setEventCallback(
        [this](WindowEvent event, const void *data) -> bool {
            if (event == WindowEvent::RESIZE && data != nullptr) {
                const auto *size =
                    static_cast<const graphics::FramebufferSize *>(data);
                Logger::info(
                    "Window resize event: {}x{}",
                    size->width,
                    size->height
                );

                if (m_gpu) {
                    Logger::debug("Triggering GPU forceResize...");
                    auto resizeResult = m_gpu->resize(
                        { .width = size->width, .height = size->height }
                    );
                    if (!resizeResult) {
                        Logger::error(
                            "Failed to force GPU resize: {}",
                            resizeResult.error()
                        );
                    } else {
                        Logger::info("GPU resize triggered successfully");
                    }
                } else {
                    Logger::error("GPU not available during window resize");
                }

                Logger::debug("Updating camera aspect ratio...");
                const f32 NEW_ASPECT = static_cast<f32>(size->width) /
                                       static_cast<f32>(size->height);

                auto aspectRatioResult = m_camera->setAspectRatio(NEW_ASPECT);

                if (!aspectRatioResult) {
                    Logger::error(
                        "Failed to set camera aspect ratio: {}",
                        aspectRatioResult.error()
                    );
                } else {
                    Logger::info(
                        "Camera aspect ratio updated to: {:.2f}",
                        NEW_ASPECT
                    );
                }

                return true;
            }
            return false;
        }
    );

    return true;
}

auto App::createGPUDevice() -> bool
{
    graphics::GPU::CreateInfo deviceInfo;
    deviceInfo.appName = "Hello cube";
    deviceInfo.appVersion = core::Version{ .major = 0, .minor = 1, .patch = 0 };
    deviceInfo.enableValidationLayers = true;
    deviceInfo.windowHandle = m_window->getNativeHandle();
    deviceInfo.width = m_window->getWidth();
    deviceInfo.height = m_window->getHeight();

    auto gpuResult = graphics::GPU::create(deviceInfo);
    if (!gpuResult) {
        Logger::error("Failed to create GPU device: {}", gpuResult.error());
        return false;
    }

    m_gpu = std::move(gpuResult.value());

    Logger::info("GPU device created successfully");
    return true;
}

auto App::createMesh() -> bool
{
    std::vector<Vertex> vertices = {
        // Front face (z = 0.5)
        { .position = { -0.5F, -0.5F, 0.5F }, .uv = { 0.0F, 0.0F } },
        { .position = { 0.5F, -0.5F, 0.5F }, .uv = { 1.0F, 0.0F } },
        { .position = { 0.5F, 0.5F, 0.5F }, .uv = { 1.0F, 1.0F } },
        { .position = { -0.5F, 0.5F, 0.5F }, .uv = { 0.0F, 1.0F } },

        // Back face (z = -0.5)
        { .position = { 0.5F, -0.5F, -0.5F }, .uv = { 0.0F, 0.0F } },
        { .position = { -0.5F, -0.5F, -0.5F }, .uv = { 1.0F, 0.0F } },
        { .position = { -0.5F, 0.5F, -0.5F }, .uv = { 1.0F, 1.0F } },
        { .position = { 0.5F, 0.5F, -0.5F }, .uv = { 0.0F, 1.0F } },

        // Left face (x = -0.5)
        { .position = { -0.5F, -0.5F, -0.5F }, .uv = { 0.0F, 0.0F } },
        { .position = { -0.5F, -0.5F, 0.5F }, .uv = { 1.0F, 0.0F } },
        { .position = { -0.5F, 0.5F, 0.5F }, .uv = { 1.0F, 1.0F } },
        { .position = { -0.5F, 0.5F, -0.5F }, .uv = { 0.0F, 1.0F } },

        // Right face (x = 0.5)
        { .position = { 0.5F, -0.5F, 0.5F }, .uv = { 0.0F, 0.0F } },
        { .position = { 0.5F, -0.5F, -0.5F }, .uv = { 1.0F, 0.0F } },
        { .position = { 0.5F, 0.5F, -0.5F }, .uv = { 1.0F, 1.0F } },
        { .position = { 0.5F, 0.5F, 0.5F }, .uv = { 0.0F, 1.0F } },

        // Bottom face (y = -0.5)
        { .position = { -0.5F, -0.5F, -0.5F }, .uv = { 0.0F, 0.0F } },
        { .position = { 0.5F, -0.5F, -0.5F }, .uv = { 1.0F, 0.0F } },
        { .position = { 0.5F, -0.5F, 0.5F }, .uv = { 1.0F, 1.0F } },
        { .position = { -0.5F, -0.5F, 0.5F }, .uv = { 0.0F, 1.0F } },

        // Top face (y = 0.5)
        { .position = { -0.5F, 0.5F, 0.5F }, .uv = { 0.0F, 0.0F } },
        { .position = { 0.5F, 0.5F, 0.5F }, .uv = { 1.0F, 0.0F } },
        { .position = { 0.5F, 0.5F, -0.5F }, .uv = { 1.0F, 1.0F } },
        { .position = { -0.5F, 0.5F, -0.5F }, .uv = { 0.0F, 1.0F } }
    };

    std::vector<u32> indices = { // Front face (z = 0.5)
                                 0,
                                 1,
                                 2,
                                 2,
                                 3,
                                 0,
                                 // Back face (z = -0.5)
                                 4,
                                 5,
                                 6,
                                 6,
                                 7,
                                 4,
                                 // Left face (x = -0.5)
                                 8,
                                 9,
                                 10,
                                 10,
                                 11,
                                 8,
                                 // Right face (x = 0.5)
                                 12,
                                 13,
                                 14,
                                 14,
                                 15,
                                 12,
                                 // Bottom face (y = -0.5)
                                 16,
                                 17,
                                 18,
                                 18,
                                 19,
                                 16,
                                 // Top face (y = 0.5)
                                 20,
                                 21,
                                 22,
                                 22,
                                 23,
                                 20
    };

    auto meshResult =
        graphics::Mesh<Vertex, u32>::create(m_gpu.get(), vertices, indices);

    if (!meshResult) {
        Logger::error("Failed to create mesh: {}", meshResult.error());
        return false;
    }

    m_mesh = std::move(meshResult.value());

    return true;
}

auto App::createTexture() -> bool
{
    auto textureResult = graphics::TextureLoader::loadFromFile(
        m_gpu.get(),
        findResourcePath("textures", "test.png")
    );

    if (!textureResult) {
        Logger::error("Failed to create texture: {}", textureResult.error());
        return false;
    }

    m_texture = std::move(textureResult.value());
    Logger::info("Texture created successfully");
    return true;
}

auto App::createPipeline() -> bool
{
    fs::path vertexShaderPath = findResourcePath("shaders", "cube.vert.spv");
    fs::path fragmentShaderPath = findResourcePath("shaders", "cube.frag.spv");

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
            auto vert = dir / "cube.vert.spv";
            auto frag = dir / "cube.frag.spv";

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
            Logger::error("Could not find shader files");
            return false;
        }
    } else {
        Logger::info("Found shaders:");
        Logger::info("  Vertex shader: {}", vertexShaderPath.string());
        Logger::info("  Fragment shader: {}", fragmentShaderPath.string());
    }

    graphics::Pipeline::CreateInfo pipelineInfo;
    pipelineInfo.name = "CubePipeline";
    pipelineInfo.vertexShader = vertexShaderPath;
    pipelineInfo.fragmentShader = fragmentShaderPath;
    pipelineInfo.primitiveTopology = graphics::PrimitiveTopology::TRIANGLE_LIST;
    pipelineInfo.polygonMode = graphics::PolygonMode::FILL;
    pipelineInfo.cullMode = graphics::CullMode::NONE;
    pipelineInfo.frontFace = graphics::FrontFace::COUNTER_CLOCKWISE;
    pipelineInfo.lineWidth = 1.0F;

    pipelineInfo.depthTest = true;
    pipelineInfo.depthWrite = true;
    pipelineInfo.depthCompareOp = graphics::CompareOp::LESS;
    pipelineInfo.stencilTest = false;

    pipelineInfo.depthFormat = graphics::ImageFormat::D32_SFLOAT;

    pipelineInfo.blend = true;
    pipelineInfo.srcColorBlendFactor = graphics::BlendFactor::SRC_ALPHA;
    pipelineInfo.dstColorBlendFactor =
        graphics::BlendFactor::ONE_MINUS_SRC_ALPHA;
    pipelineInfo.srcAlphaBlendFactor = graphics::BlendFactor::ONE;
    pipelineInfo.dstAlphaBlendFactor = graphics::BlendFactor::ZERO;
    pipelineInfo.colorBlendOp = graphics::BlendOp::ADD;
    pipelineInfo.alphaBlendOp = graphics::BlendOp::ADD;
    pipelineInfo.colorWriteMask = graphics::ColorComponentFlags::ALL;

    pipelineInfo.vertexLayout = Vertex::getLayout();

    auto pipelineResult = m_gpu->createPipeline(pipelineInfo);
    if (!pipelineResult) {
        Logger::error("Failed to create pipeline: {}", pipelineResult.error());
        return false;
    }

    m_pipeline = std::move(pipelineResult.value());
    Logger::info("Pipeline created successfully");
    return true;
}

auto App::createUBO() -> bool
{
    graphics::PerspectiveCamera::CreateInfo cameraInfo;
    cameraInfo.name = "MainCamera";
    cameraInfo.position = { 0.0F, 0.0F, 3.0F };
    cameraInfo.rotation = { 1.0F, 0.0F, 0.0F, 0.0F };
    cameraInfo.perspective.fieldOfView = 45.0F;
    cameraInfo.perspective.aspectRatio =
        static_cast<f32>(m_window->getWidth()) /
        static_cast<f32>(m_window->getHeight());
    cameraInfo.perspective.nearPlane = 0.1F;
    cameraInfo.perspective.farPlane = 100.0F;

    m_camera = graphics::PerspectiveCamera::create(cameraInfo);

    CameraUBO initialData{};
    initialData.view = m_camera->getViewMatrix();
    initialData.proj = m_camera->getProjectionMatrix();
    initialData.position = m_camera->getPosition();
    initialData.time = 0.0F;

    auto uboResult = m_gpu->createUBO<CameraUBO>(initialData);
    if (!uboResult) {
        Logger::error("Failed to create UBO: {}", uboResult.error());
        return false;
    }

    m_cameraUBO = std::move(uboResult.value());
    Logger::info("UBO created successfully");
    return true;
}

auto App::mainLoop() -> void
{
    Logger::info("Entering main render loop");

    while (m_isRunning && !m_window->shouldClose()) {
        try {
            auto frameResult = m_gpu->beginFrame();
            if (frameResult) {
                render();

                auto endResult = m_gpu->endFrame();
                if (!endResult) {
                    const auto &err = endResult.error();
                    Logger::warning(
                        "Failed to end frame: {}: {}",
                        err.context,
                        err.message
                    );
                }
                m_frameCount++;
            } else {
                const auto &err = frameResult.error();
                Logger::warning(
                    "Failed to begin frame: {}: {}",
                    err.context,
                    err.message
                );
            }
        } catch (const std::exception &e) {
            Logger::critical("Exception in main loop: {}", e.what());
            break;
        } catch (...) {
            Logger::critical("Unknown exception in main loop");
            break;
        }

        m_window->pollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    try {
        m_gpu->waitIdle();
        Logger::info("Exiting main render loop");
    } catch (const std::exception &e) {
        Logger::critical("Exception during GPU wait: {}", e.what());
    }
}

auto App::updateCamera(f32 deltaTime) -> void
{
    if (!m_camera || !m_cameraUBO) {
        return;
    }

    const f32 T = static_cast<f32>(m_frameCount) * 0.02F;
    const f32 RADIUS = 3.0F;
    const f32 HEIGHT = 1.0F;

    m_camera->setPosition(
        { RADIUS * std::cos(T),
          HEIGHT + (0.5F * std::sin(T * 2.0F)),
          RADIUS * std::sin(T) }
    );

    m_camera->lookAt({ .target = { 0.0F, 0.0F, 0.0F } });

    m_cameraUBO->view = m_camera->getViewMatrix();
    m_cameraUBO->proj = m_camera->getProjectionMatrix();
    m_cameraUBO->position = m_camera->getPosition();
    m_cameraUBO->time = T;
}

auto App::render() -> void
{
    updateCamera(0.016F);

    m_pipeline->bind();
    m_mesh->draw();
}

auto App::getExecutablePath() -> fs::path
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

auto App::findResourcePath(
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
