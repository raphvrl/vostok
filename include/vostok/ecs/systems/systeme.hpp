#pragma once

#include "vostok/core/type.hpp"

#include <string_view>

namespace vostok::ecs
{

class ECS;

class Systeme
{
public:
    Systeme() = default;
    virtual ~Systeme() = default;

    Systeme(const Systeme &) = delete;
    auto operator=(const Systeme &) -> Systeme & = delete;
    Systeme(Systeme &&) = default;
    auto operator=(Systeme &&) -> Systeme & = default;

    virtual void update(ECS &ecs, f32 deltaTime) = 0;
    virtual void render(ECS &ecs) = 0;

    [[nodiscard]] virtual auto getName() const -> std::string_view = 0;
};

} // namespace vostok::ecs