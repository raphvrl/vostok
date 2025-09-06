#pragma once

#include "vostok/core/type.hpp"
#include "vostok/ecs/systems/systeme.hpp"

#include <entt/entt.hpp>
#include <expected>
#include <memory>
#include <string>

namespace vostok::ecs
{

using Entity = entt::entity;
using Registry = entt::registry;

class ECS
{
public:
    ECS() = default;
    ~ECS() = default;

    ECS(const ECS &) = delete;
    ECS(ECS &&) = delete;
    auto operator=(const ECS &) -> ECS & = delete;
    auto operator=(ECS &&) -> ECS & = delete;

    static auto create() -> std::expected<std::unique_ptr<ECS>, std::string>;

    auto createEntity() -> Entity;
    void destroyEntity(Entity entity);
    [[nodiscard]] auto isValid(Entity entity) const -> bool;

    template <typename T>
    void addComponent(Entity entity, T &&component)
    {
        m_registry.emplace<T>(entity, std::forward<T>(component));
    }

    template <typename T>
    auto getComponent(Entity entity) -> T &
    {
        return m_registry.get<T>(entity);
    }

    template <typename T>
    [[nodiscard]] auto hasComponent(Entity entity) const -> bool
    {
        return m_registry.all_of<T>(entity);
    }

    template <typename T>
    void removeComponent(Entity entity)
    {
        m_registry.erase<T>(entity);
    }

    template <typename T, typename... Args>
    requires std::is_base_of_v<Systeme, T>
    void registerSystem(Args &&...args)
    {
        m_systems.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void update(f32 deltaTime);
    void render();

    auto getRegistry() -> Registry & { return m_registry; }
    [[nodiscard]] auto getRegistry() const -> const Registry &
    {
        return m_registry;
    }

    template <typename... Components>
    auto view()
    {
        return m_registry.view<Components...>();
    }

private:
    Registry m_registry;
    std::vector<std::unique_ptr<Systeme>> m_systems;
};

} // namespace vostok::ecs