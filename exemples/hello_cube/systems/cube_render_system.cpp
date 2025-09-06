#include "cube_render_system.hpp"

#include "../components/cube_component.hpp"
#include "vostok/ecs/ecs.hpp"
#include "vostok/graphics/pipeline.hpp"
#include "vostok/math/matrix_ops.hpp"

using namespace vostok;
using namespace vostok::ecs;
using namespace vostok::graphics;

CubeRenderSystem::CubeRenderSystem(
    TextureManagerHandle *textureManager,
    PipelineHandle *pipeline
)
    : m_textureManager(textureManager),
      m_pipeline(pipeline)
{}

void CubeRenderSystem::update(ECS &ecs, f32 deltaTime) {}

void CubeRenderSystem::render(ECS &ecs)
{
    auto view = ecs.view<TransformComponent, CubeComponent>();

    for (auto [entity, transform, cube] : view.each()) {
        math::Mat4 modelMatrix = calculateModelMatrix(transform);

        if (!cube.m_texturePath.empty()) {
            auto textureResult =
                m_textureManager->getOrLoadTexture(cube.m_texturePath);
            if (textureResult) {
                renderTexturedCube(modelMatrix, cube);
            } else {
                renderColoredCube(modelMatrix, cube);
            }
        } else {
            renderColoredCube(modelMatrix, cube);
        }
    }
}

auto CubeRenderSystem::calculateModelMatrix(const TransformComponent &transform)
    -> math::Mat4
{
    return math::translate(math::Mat4{ 1.0F }, transform.position) *
           math::rotate(
               math::Mat4{ 1.0F },
               transform.rotation.x,
               math::Vec3{ 1.0F, 0.0F, 0.0F }
           ) *
           math::rotate(
               math::Mat4{ 1.0F },
               transform.rotation.y,
               math::Vec3{ 0.0F, 1.0F, 0.0F }
           ) *
           math::rotate(
               math::Mat4{ 1.0F },
               transform.rotation.z,
               math::Vec3{ 0.0F, 0.0F, 1.0F }
           ) *
           math::scale(math::Mat4{ 1.0F }, transform.scale);
}

void CubeRenderSystem::renderTexturedCube(
    const math::Mat4 &modelMatrix,
    CubeComponent &cube
)
{
    m_pipeline->bind();

    if (auto result = m_pipeline->push(modelMatrix); !result) {
        Logger::error(
            "Failed to push model matrix to pipeline: {}",
            result.error()
        );
    }

    cube.m_mesh->draw();
}

void CubeRenderSystem::renderColoredCube(
    const math::Mat4 &modelMatrix,
    CubeComponent &cube
)
{
    m_pipeline->bind();

    if (auto result = m_pipeline->push(modelMatrix); !result) {
        Logger::error(
            "Failed to push model matrix to pipeline: {}",
            result.error()
        );
    }

    cube.m_mesh->draw();
}