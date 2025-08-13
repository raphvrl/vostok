#include "graphics/camera/frustum_camera.hpp"

#include "core/logger/logger.hpp"
#include "math/functions.hpp"
#include "math/projections.hpp"
#include "math/types.hpp"

#include <format>

namespace vostok::graphics
{

FrustumCamerahandle::FrustumCamerahandle(const CreateInfo &createInfo)
    : Camera(createInfo),
      m_config(createInfo.config)
{
    if (auto result = validateConfig(m_config); !result.has_value()) {
        Logger::error(
            "FrustumCamera '{}' created with invalid config: {}",
            getName(),
            result.error()
        );
        Logger::warning(
            "FrustumCamera '{}' falling back to default configuration",
            getName()
        );
        m_config = FrustumConfig{};
    } else {
        Logger::info(
            "FrustumCamera '{}' created - Bounds: L:{:.2f} R:{:.2f} B:{:.2f} "
            "T:{:.2f}, Near:{:.3f} "
            "Far:{:.1f}",
            getName(),
            m_config.left,
            m_config.right,
            m_config.bottom,
            m_config.top,
            m_config.nearPlane,
            m_config.farPlane
        );
    }

    markProjectionDirty();
}

auto FrustumCamerahandle::getProjectionMatrix() const noexcept
    -> const math::Mat4 &
{
    if (m_projectionMatrixDirty) {
        updateProjectionMatrix();
    }

    return m_projectionMatrix;
}

auto FrustumCamerahandle::getViewMatrix() const noexcept -> const math::Mat4 &
{
    return Camera::getViewMatrix();
}

auto FrustumCamerahandle::getViewProjectionMatrix() const -> math::Mat4
{
    return getProjectionMatrix() * getViewMatrix();
}

auto FrustumCamerahandle::updateConfig(const FrustumConfig &config) noexcept
    -> std::expected<void, std::string>
{
    if (auto result = validateConfig(config); !result.has_value()) {
        return std::unexpected(result.error());
    }

    const bool CHANGED =
        (m_config.left != config.left) || (m_config.right != config.right) ||
        (m_config.bottom != config.bottom) || (m_config.top != config.top) ||
        (m_config.nearPlane != config.nearPlane) ||
        (m_config.farPlane != config.farPlane);

    if (CHANGED) {
        Logger::debug("FrustumCamera '{}' config updated", getName());
        m_config = config;
        onProjectionChanged();
    }

    return {};
}

auto FrustumCamerahandle::setBounds(
    f32 left,
    f32 right,
    f32 bottom,
    f32 top
) noexcept -> std::expected<void, std::string>
{
    if (left >= right) {
        return std::unexpected(
            std::format("Left ({}) must be less than right ({}).", left, right)
        );
    }

    if (bottom >= top) {
        return std::unexpected(
            std::format("Bottom ({}) must be less than top ({}).", bottom, top)
        );
    }

    const bool CHANGED = (m_config.left != left) || (m_config.right != right) ||
                         (m_config.bottom != bottom) || (m_config.top != top);

    if (CHANGED) {
        Logger::debug(
            "FrustumCamera '{}' bounds changed: [{:.2f}, {:.2f}, {:.2f}, "
            "{:.2f}] -> [{:.2f}, "
            "{:.2f}, {:.2f}, {:.2f}]",
            getName(),
            m_config.left,
            m_config.right,
            m_config.bottom,
            m_config.top,
            left,
            right,
            bottom,
            top
        );

        m_config.left = left;
        m_config.right = right;
        m_config.bottom = bottom;
        m_config.top = top;
        onProjectionChanged();
    }

    return {};
}

auto FrustumCamerahandle::setPlanes(f32 nearPlane, f32 farPlane) noexcept
    -> std::expected<void, std::string>
{
    if (nearPlane <= 0.0F) {
        return std::unexpected(
            std::format("Near plane ({}) must be greater than zero.", nearPlane)
        );
    }

    if (nearPlane >= farPlane) {
        return std::unexpected(
            std::format(
                "Near plane ({}) must be less than far plane ({}).",
                nearPlane,
                farPlane
            )
        );
    }

    const bool CHANGED =
        (m_config.nearPlane != nearPlane) || (m_config.farPlane != farPlane);

    if (CHANGED) {
        Logger::debug(
            "FrustumCamera '{}' planes changed: Near {:.3f} -> {:.3f}, Far "
            "{:.1f} -> {:.1f}",
            getName(),
            m_config.nearPlane,
            nearPlane,
            m_config.farPlane,
            farPlane
        );

        m_config.nearPlane = nearPlane;
        m_config.farPlane = farPlane;
        onProjectionChanged();
    }

    return {};
}

auto FrustumCamerahandle::create(const CreateInfo &createInfo)
    -> std::unique_ptr<FrustumCamerahandle>
{
    return std::make_unique<FrustumCamerahandle>(createInfo);
}

auto FrustumCamerahandle::createFromPerspective(
    const PerspectiveCameraHandle &perspectiveCamera
) noexcept -> std::unique_ptr<FrustumCamerahandle>
{
    const auto FOV = perspectiveCamera.getFieldOfView();
    const auto ASPECT_RATIO = perspectiveCamera.getAspectRatio();
    const auto NEAR_PLANE = perspectiveCamera.getNearPlane();
    const auto FAR_PLANE = perspectiveCamera.getFarPlane();

    const f32 FOV_RAD = math::radians(FOV);
    const f32 TAN_HALF_FOV = std::tan(FOV_RAD * 0.5F);

    const f32 TOP = NEAR_PLANE * TAN_HALF_FOV;
    const f32 BOTTOM = -TOP;
    const f32 RIGHT = TOP * ASPECT_RATIO;
    const f32 LEFT = -RIGHT;

    FrustumConfig frustumConfig{ .left = LEFT,
                                 .right = RIGHT,
                                 .bottom = BOTTOM,
                                 .top = TOP,
                                 .nearPlane = NEAR_PLANE,
                                 .farPlane = FAR_PLANE };

    CreateInfo createInfo{};
    createInfo.config = frustumConfig;

    Logger::info(
        "FrustumCamera created from PerspectiveCamera '{}' - FOV:{:.1f}° -> "
        "Bounds:[{:.2f}, "
        "{:.2f}, {:.2f}, {:.2f}]",
        perspectiveCamera.getName(),
        FOV,
        LEFT,
        RIGHT,
        BOTTOM,
        TOP
    );

    return std::make_unique<FrustumCamerahandle>(createInfo);
}

void FrustumCamerahandle::onTransformChanged() noexcept {}

void FrustumCamerahandle::onProjectionChanged() noexcept
{
    markProjectionDirty();
}

void FrustumCamerahandle::updateProjectionMatrix() const noexcept
{
    Logger::trace("FrustumCamera '{}' updating projection matrix", getName());

    m_projectionMatrix = math::frustum(
        m_config.left,
        m_config.right,
        m_config.bottom,
        m_config.top,
        m_config.nearPlane,
        m_config.farPlane
    );

    m_projectionMatrixDirty = false;
}

auto FrustumCamerahandle::validateConfig(const FrustumConfig &config) noexcept
    -> std::expected<void, std::string>
{
    if (config.left >= config.right) {
        return std::unexpected(
            std::format(
                "Left ({}) must be less than right ({}).",
                config.left,
                config.right
            )
        );
    }

    if (config.bottom >= config.top) {
        return std::unexpected(
            std::format(
                "Bottom ({}) must be less than top ({}).",
                config.bottom,
                config.top
            )
        );
    }

    if (config.nearPlane <= 0.0F) {
        return std::unexpected(
            std::format(
                "Near plane ({}) must be greater than zero.",
                config.nearPlane
            )
        );
    }

    if (config.nearPlane >= config.farPlane) {
        return std::unexpected(
            std::format(
                "Near plane ({}) must be less than far plane ({}).",
                config.nearPlane,
                config.farPlane
            )
        );
    }

    return {};
}

void FrustumCamerahandle::markProjectionDirty() noexcept
{
    m_projectionMatrixDirty = true;
}

} // namespace vostok::graphics