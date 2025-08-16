#pragma once

#include "vostok/graphics/buffers/ibo.hpp"
#include "vostok/graphics/buffers/texture.hpp"
#include "vostok/graphics/buffers/ubo.hpp"
#include "vostok/graphics/buffers/vbo.hpp"
#include "vostok/graphics/camera/perspective_camera.hpp"
#include "vostok/graphics/gpu.hpp"
#include "vostok/graphics/pipeline.hpp"
#include "vostok/math/types.hpp"
#include "vostok/window/window.hpp"

#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

using namespace vostok;

struct CameraUBO
{
    alignas(16) math::Mat4 view;
    alignas(16) math::Mat4 proj;
    alignas(16) math::Vec3 position;
    alignas(4) f32 time;
};

struct Vertex
{
    math::Vec3 position;
    math::Vec2 uv;
};

class App
{
public:
    App();
    ~App();

    App(const App &) = delete;
    auto operator=(const App &) -> App & = delete;
    App(App &&) = delete;
    auto operator=(App &&) -> App & = delete;

    auto initialize() -> bool;
    auto run() -> void;
    auto shutdown() -> void;

private:
    auto createWindow() -> bool;
    auto createGPUDevice() -> bool;
    auto createVertexBuffer() -> bool;
    auto createIndexBuffer() -> bool;
    auto createTexture() -> bool;
    auto createPipeline() -> bool;
    auto createUBO() -> bool;

    auto mainLoop() -> void;
    auto updateCamera(f32 deltaTime) -> void;
    auto render() -> void;

    static auto
    findResourcePath(const fs::path &resourceType, const fs::path &resourceName)
        -> fs::path;
    static auto getExecutablePath() -> fs::path;

    Window m_window;
    graphics::GPU m_gpu;

    graphics::VBO<Vertex> m_vertexBuffer;
    graphics::IBO<u32> m_indexBuffer;
    graphics::Pipeline m_pipeline;

    graphics::UBO<CameraUBO> m_cameraUBO;
    graphics::Texture m_texture;
    graphics::PerspectiveCamera m_camera;

    bool m_isRunning = false;
    uint32_t m_frameCount = 0;
    std::chrono::high_resolution_clock::time_point m_startTime;
};