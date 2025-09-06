#pragma once

#include "../components/cube_component.hpp"
#include "vostok/ecs/components/transform_component.hpp"
#include "vostok/ecs/systems/systeme.hpp"
#include "vostok/graphics/pipeline.hpp"
#include "vostok/graphics/textures/texture_manager.hpp"

using namespace vostok;
using namespace vostok::graphics;
using namespace vostok::ecs;

class CubeRenderSystem : public Systeme
{
public:
    CubeRenderSystem(
        TextureManagerHandle *textureManager,
        PipelineHandle *pipeline
    );
    ~CubeRenderSystem() = default;

    CubeRenderSystem(const CubeRenderSystem &) = delete;
    auto operator=(const CubeRenderSystem &) -> CubeRenderSystem & = delete;
    CubeRenderSystem(CubeRenderSystem &&) = delete;
    auto operator=(CubeRenderSystem &&) -> CubeRenderSystem & = delete;

    auto update(ECS &ecs, f32 deltaTime) -> void override;
    auto render(ECS &ecs) -> void override;

    [[nodiscard]] auto getName() const -> std::string_view override
    {
        return "CubeRenderSystem";
    }

private:
    void renderColoredCube(const math::Mat4 &modelMatrix, CubeComponent &cube);

    void renderTexturedCube(const math::Mat4 &modelMatrix, CubeComponent &cube);

    static auto calculateModelMatrix(const TransformComponent &transform)
        -> math::Mat4;

    TextureManagerHandle *m_textureManager;
    PipelineHandle *m_pipeline;
};