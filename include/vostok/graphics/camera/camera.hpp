#pragma once

#include "vostok/core/type.hpp"
#include "vostok/math/types.hpp"

#include <string>

namespace vostok::graphics
{

static constexpr math::Vec3 WORLD_UP{ 0.0F, 1.0F, 0.0F };

enum class CameraType : u8
{
    PERSPECTIVE,
    ORTHOGRAPHIC,
    FRUSTUM,
};

struct LookAtParams
{
    math::Vec3 target{ 0.0F, 0.0F, 0.0F };
    math::Vec3 up{ WORLD_UP };
};

class Camera
{
public:
    struct CreateInfo
    {
        std::string name = "Camera";
        math::Vec3 position{ 0.0F, 0.0F, 0.0F };
        math::Quat rotation{ 1.0F, 0.0F, 0.0F, 0.0F };
    };

    explicit Camera(const CreateInfo &createInfo);
    virtual ~Camera() = default;

    Camera(const Camera &) = delete;
    auto operator=(const Camera &) -> Camera & = delete;
    Camera(Camera &&) = default;
    auto operator=(Camera &&) -> Camera & = default;

    [[nodiscard]] virtual auto getProjectionMatrix() const noexcept -> const math::Mat4 & = 0;
    [[nodiscard]] virtual auto getViewMatrix() const noexcept -> const math::Mat4 & = 0;

    [[nodiscard]] virtual auto getViewProjectionMatrix() const -> math::Mat4 = 0;

    [[nodiscard]] virtual auto getCameraType() const noexcept -> CameraType = 0;

    void setPosition(const math::Vec3 &position) noexcept;
    void setRotation(const math::Quat &rotation) noexcept;

    void setName(const std::string &name) noexcept;

    [[nodiscard]] auto getForward() const noexcept -> math::Vec3;
    [[nodiscard]] auto getRight() const noexcept -> math::Vec3;
    [[nodiscard]] auto getUp() const noexcept -> math::Vec3;

    void translate(const math::Vec3 &delta) noexcept;
    void rotate(const math::Quat &delta) noexcept;
    void lookAt(const LookAtParams &params) noexcept;

    void moveForward(f32 distance) noexcept;
    void moveRight(f32 distance) noexcept;
    void moveUp(f32 distance) noexcept;

    [[nodiscard]] auto getName() const noexcept -> const std::string & { return m_name; }
    [[nodiscard]] auto getPosition() const noexcept -> const math::Vec3 & { return m_position; }
    [[nodiscard]] auto getRotation() const noexcept -> const math::Quat & { return m_rotation; }

protected:
    virtual void onTransformChanged() noexcept = 0;
    virtual void onProjectionChanged() noexcept = 0;

private:
    std::string m_name;
    math::Vec3 m_position{ 0.0F, 0.0F, 0.0F };
    math::Quat m_rotation{ 1.0F, 0.0F, 0.0F, 0.0F };

    mutable bool m_viewMatrixDirty = true;
    mutable bool m_projectionMatrixDirty = true;
    mutable math::Mat4 m_viewMatrix{ 1.0F };

    void updateViewMatrix() const noexcept;
    void markDirty() noexcept;
};

} // namespace vostok::graphics