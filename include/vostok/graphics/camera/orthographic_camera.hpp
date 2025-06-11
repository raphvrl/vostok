#pragma once

#include "camera.hpp"

#include <expected>
#include <string>

namespace vostok::graphics
{

struct CenteredParams
{
    f32 width;
    f32 height;
    f32 nearPlane = -100.0F;
    f32 farPlane = 100.0F;
};

class OrthographicCamera : public Camera
{
public:
    struct OrthographicConfig
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
        OrthographicConfig config{};
    };

    explicit OrthographicCamera(const CreateInfo &createInfo);

    [[nodiscard]] auto getProjectionMatrix() const noexcept -> const math::Mat4 & override;
    [[nodiscard]] auto getViewMatrix() const noexcept -> const math::Mat4 & override;
    [[nodiscard]] auto getViewProjectionMatrix() const -> math::Mat4 override;
    [[nodiscard]] auto getCameraType() const noexcept -> CameraType override
    {
        return CameraType::ORTHOGRAPHIC;
    }

    [[nodiscard]] auto setBounds(f32 left, f32 right, f32 bottom, f32 top) noexcept
        -> std::expected<void, std::string>;
    [[nodiscard]] auto setPlanes(f32 nearPlane, f32 farPlane) noexcept
        -> std::expected<void, std::string>;
    [[nodiscard]] auto updateConfig(const OrthographicConfig &config) noexcept
        -> std::expected<void, std::string>;

    [[nodiscard]] auto getLeft() const noexcept -> f32 { return m_config.left; }
    [[nodiscard]] auto getRight() const noexcept -> f32 { return m_config.right; }
    [[nodiscard]] auto getBottom() const noexcept -> f32 { return m_config.bottom; }
    [[nodiscard]] auto getTop() const noexcept -> f32 { return m_config.top; }
    [[nodiscard]] auto getNearPlane() const noexcept -> f32 { return m_config.nearPlane; }
    [[nodiscard]] auto getFarPlane() const noexcept -> f32 { return m_config.farPlane; }

    [[nodiscard]] auto getWidth() const noexcept -> f32 { return m_config.right - m_config.left; }
    [[nodiscard]] auto getHeight() const noexcept -> f32 { return m_config.top - m_config.bottom; }
    [[nodiscard]] auto getAspectRatio() const noexcept -> f32;

    [[nodiscard]] static auto createCentered(const CenteredParams &params) -> OrthographicCamera;
    [[nodiscard]] static auto createUI(f32 screenWidth, f32 screenHeight) -> OrthographicCamera;
    [[nodiscard]] static auto createFromBounds(f32 left, f32 right, f32 bottom, f32 top)
        -> OrthographicCamera;

protected:
    void onTransformChanged() noexcept override;
    void onProjectionChanged() noexcept override;

private:
    OrthographicConfig m_config{};
    mutable math::Mat4 m_projectionMatrix{ 1.0F };
    mutable bool m_projectionMatrixDirty = true;

    void updateProjectionMatrix() const noexcept;
    [[nodiscard]] static auto validateConfig(const OrthographicConfig &config) noexcept
        -> std::expected<bool, std::string>;
    void markProjectionDirty() noexcept;
};

} // namespace vostok::graphics
