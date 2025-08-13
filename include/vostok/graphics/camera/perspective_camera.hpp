#pragma once

#include "camera.hpp"

#include <array>
#include <expected>
#include <memory>

namespace vostok::graphics
{

class PerspectiveCameraHandle final : public Camera
{
public:
    struct PerspectiveConfig
    {
        f32 fieldOfView = 45.0F;
        f32 aspectRatio = 16.0F / 9.0F;
        f32 nearPlane = 0.1F;
        f32 farPlane = 1000.0F;
        bool infiniteFarPlane = false;
    };

    struct CreateInfo : Camera::CreateInfo
    {
        PerspectiveConfig perspective{};
    };

    explicit PerspectiveCameraHandle(const CreateInfo &createInfo);

    [[nodiscard]] auto getProjectionMatrix() const noexcept
        -> const math::Mat4 & override;
    [[nodiscard]] auto getViewMatrix() const noexcept
        -> const math::Mat4 & override;
    [[nodiscard]] auto getViewProjectionMatrix() const -> math::Mat4 override;
    [[nodiscard]] auto getCameraType() const noexcept -> CameraType override
    {
        return CameraType::PERSPECTIVE;
    }

    [[nodiscard]] auto setFieldOfView(f32 fieldOfView) noexcept
        -> std::expected<void, std::string>;
    [[nodiscard]] auto setAspectRatio(f32 aspectRatio) noexcept
        -> std::expected<void, std::string>;
    [[nodiscard]] auto setNearPlane(f32 nearPlane) noexcept
        -> std::expected<void, std::string>;
    [[nodiscard]] auto setFarPlane(f32 farPlane) noexcept
        -> std::expected<void, std::string>;
    void setInfiniteFarPlane(bool infinite) noexcept;

    [[nodiscard]] auto updateConfig(const PerspectiveConfig &config) noexcept
        -> std::expected<void, std::string>;

    [[nodiscard]] auto getFieldOfView() const noexcept -> f32
    {
        return m_config.fieldOfView;
    }
    [[nodiscard]] auto getAspectRatio() const noexcept -> f32
    {
        return m_config.aspectRatio;
    }
    [[nodiscard]] auto getNearPlane() const noexcept -> f32
    {
        return m_config.nearPlane;
    }
    [[nodiscard]] auto getFarPlane() const noexcept -> f32
    {
        return m_config.farPlane;
    }
    [[nodiscard]] auto isInfiniteFarPlane() const noexcept -> bool
    {
        return m_config.infiniteFarPlane;
    }

    [[nodiscard]] auto getFrustumCorners() const -> std::array<math::Vec3, 8>;
    [[nodiscard]] auto getWorldToScreenRay(
        const math::Vec2 &screenPos,
        const math::Vec2 &screenSize
    ) const -> std::pair<math::Vec3, math::Vec3>;

    [[nodiscard]] static auto createDefault() -> PerspectiveCameraHandle;
    [[nodiscard]] static auto createWithFOV(f32 fov, f32 aspectRatio)
        -> PerspectiveCameraHandle;

protected:
    void onTransformChanged() noexcept override;
    void onProjectionChanged() noexcept override;

private:
    PerspectiveConfig m_config{};
    mutable math::Mat4 m_projectionMatrix{ 1.0F };
    mutable bool m_projectionMatrixDirty = true;

    void updateProjectionMatrix() const noexcept;
    [[nodiscard]] static auto
    validateConfig(const PerspectiveConfig &config) noexcept
        -> std::expected<bool, std::string>;
    void markProjectionDirty() noexcept;
};

using PerspectiveCamera = std::unique_ptr<PerspectiveCameraHandle>;

} // namespace vostok::graphics