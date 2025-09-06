#pragma once

#include "systems/cube_render_system.hpp"
#include "vostok/ecs/ecs.hpp"
#include "vostok/graphics/buffers/ubo.hpp"
#include "vostok/graphics/camera/perspective_camera.hpp"
#include "vostok/graphics/gpu.hpp"
#include "vostok/graphics/mesh.hpp"
#include "vostok/graphics/pipeline.hpp"
#include "vostok/graphics/textures/texture_manager.hpp"
#include "vostok/graphics/vertex_types.hpp"
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

    static auto getLayout() -> const graphics::VertexLayout &
    {
        static const auto LAYOUT = graphics::createVertexLayout(
            { graphics::formats::VEC3, graphics::formats::VEC2 }
        );
        return LAYOUT;
    }
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

private:
    static auto createProfiler() -> bool;
    auto createResourcesPaths() -> bool;
    auto createWindow() -> bool;
    auto createGPUDevice() -> bool;
    auto createMesh() -> bool;
    auto createTextureManager() -> bool;
    auto createPipeline() -> bool;
    auto createUBO() -> bool;
    auto createECS() -> bool;
    auto createCubeEntity() -> bool;

    auto mainLoop() -> void;
    auto updateCamera(f32 deltaTime) -> void;
    auto render() -> void;

    static auto
    findResourcePath(const fs::path &resourceType, const fs::path &resourceName)
        -> fs::path;
    static auto getExecutablePath() -> fs::path;

    fs::path m_texturePath;
    fs::path m_vertexShaderPath;
    fs::path m_fragmentShaderPath;

    Window m_window;
    graphics::GPU m_gpu;

    graphics::Mesh<PositionNormalUV, u32> m_mesh;
    graphics::Pipeline m_pipeline;

    graphics::UBO<CameraUBO> m_cameraUBO;
    graphics::TextureManager m_textureManager;
    graphics::PerspectiveCamera m_camera;

    std::unique_ptr<ecs::ECS> m_ecs;

    bool m_isRunning = false;
    u32 m_frameCount = 0;
    std::chrono::high_resolution_clock::time_point m_startTime;
};