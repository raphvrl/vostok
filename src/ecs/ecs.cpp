#include "ecs/ecs.hpp"

namespace vostok::ecs
{

auto ECS::create() -> std::expected<std::unique_ptr<ECS>, std::string>
{
    return std::make_unique<ECS>();
}

auto ECS::createEntity() -> Entity
{
    return m_registry.create();
}

void ECS::destroyEntity(Entity entity)
{
    m_registry.destroy(entity);
}

auto ECS::isValid(Entity entity) const -> bool
{
    return m_registry.valid(entity);
}

void ECS::update(f32 deltaTime)
{
    for (auto &system : m_systems) {
        system->update(*this, deltaTime);
    }
}

void ECS::render()
{
    for (auto &system : m_systems) {
        system->render(*this);
    }
}

} // namespace vostok::ecs