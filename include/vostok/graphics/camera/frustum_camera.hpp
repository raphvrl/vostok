#pragma once

#include "perspective_camera.hpp"

#include <expected>
#include <memory>

namespace vostok::graphics
{

class FrustumCamerahandle : public Camera
{
public:
    struct FrustumConfig
    {
        f32 left = -1.0F;
        f32 right = 1.0F;
        f32 bottom = -1.0F;
        f32 top = 1.0F;
        f32 nearPlane = 0.1F;
        f32 farPlane = 1000.0F;
    };

    struct CreateInfo : public Camera::CreateInfo
    {
        FrustumConfig config{};
    };

    explicit FrustumCamerahandle(const CreateInfo &createInfo);

    [[nodiscard]] auto getProjectionMatrix() const noexcept
        -> const math::Mat4 & override;
    [[nodiscard]] auto getViewMatrix() const noexcept
        -> const math::Mat4 & override;
    [[nodiscard]] auto getViewProjectionMatrix() const -> math::Mat4 override;
    [[nodiscard]] auto getCameraType() const noexcept -> CameraType override
    {
        return CameraType::FRUSTUM;
    }

    [[nodiscard]] auto updateConfig(const FrustumConfig &config) noexcept
        -> std::expected<void, std::string>;
    [[nodiscard]] auto
    setBounds(f32 left, f32 right, f32 bottom, f32 top) noexcept
        -> std::expected<void, std::string>;
    [[nodiscard]] auto setPlanes(f32 nearPlane, f32 farPlane) noexcept
        -> std::expected<void, std::string>;

    [[nodiscard]] auto getLeft() const noexcept -> f32 { return m_config.left; }
    [[nodiscard]] auto getRight() const noexcept -> f32
    {
        return m_config.right;
    }
    [[nodiscard]] auto getBottom() const noexcept -> f32
    {
        return m_config.bottom;
    }
    [[nodiscard]] auto getTop() const noexcept -> f32 { return m_config.top; }
    [[nodiscard]] auto getNearPlane() const noexcept -> f32
    {
        return m_config.nearPlane;
    }
    [[nodiscard]] auto getFarPlane() const noexcept -> f32
    {
        return m_config.farPlane;
    }

    [[nodiscard]] static auto createFromPerspective(
        const PerspectiveCameraHandle &perspectiveCamera
    ) noexcept -> FrustumCamerahandle;

protected:
    void onTransformChanged() noexcept override;
    void onProjectionChanged() noexcept override;

private:
    FrustumConfig m_config{};
    mutable math::Mat4 m_projectionMatrix{};
    mutable bool m_projectionMatrixDirty = true;

    void updateProjectionMatrix() const noexcept;
    [[nodiscard]] static auto
    validateConfig(const FrustumConfig &config) noexcept
        -> std::expected<void, std::string>;
    void markProjectionDirty() noexcept;
};

using FrustumCamera = std::unique_ptr<FrustumCamerahandle>;

} // namespace vostok::graphics