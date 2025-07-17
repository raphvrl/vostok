#include "graphics/camera/perspective_camera.hpp"

#include "math/functions.hpp"
#include "math/types.hpp"

#include <algorithm>
#include <format>

namespace vostok::graphics
{

PerspectiveCamera::PerspectiveCamera(const CreateInfo &createInfo)
    : Camera(createInfo),
      m_config(createInfo.perspective)
{
    if (auto result = validateConfig(m_config); !result.has_value()) {
        m_config = PerspectiveConfig{};
    }

    markProjectionDirty();
}

auto PerspectiveCamera::getProjectionMatrix() const noexcept
    -> const math::Mat4 &
{
    if (m_projectionMatrixDirty) {
        updateProjectionMatrix();
    }

    return m_projectionMatrix;
}

auto PerspectiveCamera::getViewMatrix() const noexcept -> const math::Mat4 &
{
    return Camera::getViewMatrix();
}

auto PerspectiveCamera::getViewProjectionMatrix() const -> math::Mat4
{
    return getProjectionMatrix() * getViewMatrix();
}

auto PerspectiveCamera::setFieldOfView(f32 fieldOfView) noexcept
    -> std::expected<void, std::string>
{
    if (fieldOfView <= 0.0F || fieldOfView >= 180.0F) {
        return std::unexpected(
            std::format(
                "Invalid field of view: {}. Must be in (0, 180) degrees.",
                fieldOfView
            )
        );
    }

    if (m_config.fieldOfView != fieldOfView) {
        m_config.fieldOfView = fieldOfView;
        onProjectionChanged();
    }

    return {};
}

auto PerspectiveCamera::setAspectRatio(f32 aspectRatio) noexcept
    -> std::expected<void, std::string>
{
    if (aspectRatio <= 0.0F) {
        return std::unexpected(
            std::format(
                "Invalid aspect ratio: {}. Must be greater than zero.",
                aspectRatio
            )
        );
    }

    if (m_config.aspectRatio != aspectRatio) {
        m_config.aspectRatio = aspectRatio;
        onProjectionChanged();
    }

    return {};
}

auto PerspectiveCamera::setNearPlane(f32 nearPlane) noexcept
    -> std::expected<void, std::string>
{
    if (nearPlane <= 0.0F) {
        return std::unexpected(
            std::format(
                "Invalid near plane: {}. Must be greater than zero.",
                nearPlane
            )
        );
    }

    if (!m_config.infiniteFarPlane && nearPlane >= m_config.farPlane) {
        return std::unexpected(
            std::format(
                "Near plane ({}) must be less than far plane ({}).",
                nearPlane,
                m_config.farPlane
            )
        );
    }

    if (m_config.nearPlane != nearPlane) {
        m_config.nearPlane = nearPlane;
        onProjectionChanged();
    }

    return {};
}

auto PerspectiveCamera::setFarPlane(f32 farPlane) noexcept
    -> std::expected<void, std::string>
{
    if (m_config.infiniteFarPlane) {
        return std::unexpected(
            "Cannot set far plane when infinite far plane is enabled."
        );
    }

    if (farPlane <= m_config.nearPlane) {
        return std::unexpected(
            std::format(
                "Far plane ({}) must be greater than near plane ({}).",
                farPlane,
                m_config.nearPlane
            )
        );
    }

    if (m_config.farPlane != farPlane) {
        m_config.farPlane = farPlane;
        onProjectionChanged();
    }

    return {};
}

void PerspectiveCamera::setInfiniteFarPlane(bool infinite) noexcept
{
    if (m_config.infiniteFarPlane != infinite) {
        m_config.infiniteFarPlane = infinite;
        onProjectionChanged();
    }
}

auto PerspectiveCamera::updateConfig(const PerspectiveConfig &config) noexcept
    -> std::expected<void, std::string>
{
    if (auto result = validateConfig(config); !result.has_value()) {
        return std::unexpected(result.error());
    }

    const bool CHANGED = (m_config.fieldOfView != config.fieldOfView) ||
                         (m_config.aspectRatio != config.aspectRatio) ||
                         (m_config.nearPlane != config.nearPlane) ||
                         (m_config.farPlane != config.farPlane) ||
                         (m_config.infiniteFarPlane != config.infiniteFarPlane);

    if (CHANGED) {
        m_config = config;
        onProjectionChanged();
    }

    return {};
}

auto PerspectiveCamera::getFrustumCorners() const -> std::array<math::Vec3, 8>
{
    const auto INV_VIEW_PROJ =
        math::inverse(getProjectionMatrix() * getViewMatrix());

    static const std::array<math::Vec4, 8> NDC_CORNERS = {
        math::Vec4{ -1.0F, -1.0F, -1.0F, 1.0F },
        math::Vec4{ 1.0F, -1.0F, -1.0F, 1.0F },
        math::Vec4{ 1.0F, 1.0F, -1.0F, 1.0F },
        math::Vec4{ -1.0F, 1.0F, -1.0F, 1.0F },
        math::Vec4{ -1.0F, -1.0F, 1.0F, 1.0F },
        math::Vec4{ 1.0F, -1.0F, 1.0F, 1.0F },
        math::Vec4{ 1.0F, 1.0F, 1.0F, 1.0F },
        math::Vec4{ -1.0F, 1.0F, 1.0F, 1.0F }
    };

    std::array<math::Vec3, 8> worldCorners{};

    auto transformToWorld =
        [&INV_VIEW_PROJ](const math::Vec4 &corner) -> math::Vec3 {
        auto worldPos = INV_VIEW_PROJ * corner;
        if (worldPos.w != 0.0F) [[likely]] {
            worldPos /= worldPos.w;
        }
        return { worldPos.x, worldPos.y, worldPos.z };
    };

    std::ranges::transform(NDC_CORNERS, worldCorners.begin(), transformToWorld);

    return worldCorners;
}

auto PerspectiveCamera::getWorldToScreenRay(
    const math::Vec2 &screenPos,
    const math::Vec2 &screenSize
) const -> std::pair<math::Vec3, math::Vec3>
{
    const math::Vec2 NDC = { (2.0F * screenPos.x) / (screenSize.x - 1.0F),
                             1.0F - ((2.0F * screenPos.y) / screenSize.y) };

    const math::Vec4 NEAR_POINT = { NDC.x, NDC.y, -1.0F, 1.0F };

    const math::Vec4 FAR_POINT = { NDC.x, NDC.y, 1.0F, 1.0F };

    const auto INV_VIEW_PROJ =
        math::inverse(getProjectionMatrix() * getViewMatrix());

    auto worldNear = INV_VIEW_PROJ * NEAR_POINT;
    auto worldFar = INV_VIEW_PROJ * FAR_POINT;

    if (worldNear.w != 0.0F) {
        worldNear /= worldNear.w;
    }

    if (worldFar.w != 0.0F) {
        worldFar /= worldFar.w;
    }

    const math::Vec3 RAY_ORIGIN = { worldNear.x, worldNear.y, worldNear.z };
    const math::Vec3 RAY_END = { worldFar.x, worldFar.y, worldFar.z };
    const math::Vec3 RAY_DIRECTION = math::normalize(RAY_END - RAY_ORIGIN);

    return { RAY_ORIGIN, RAY_DIRECTION };
}

auto PerspectiveCamera::createDefault() -> PerspectiveCamera
{
    PerspectiveConfig defaultConfig{};
    CreateInfo createInfo{};
    createInfo.name = "PerspectiveCamera";
    createInfo.position = { 0.0F, 0.0F, 0.0F };
    createInfo.rotation = { 1.0F, 0.0F, 0.0F, 0.0F };
    createInfo.perspective = defaultConfig;

    return PerspectiveCamera{ createInfo };
}

auto PerspectiveCamera::createWithFOV(f32 fov, f32 aspectRatio)
    -> PerspectiveCamera
{
    CreateInfo createInfo{};
    createInfo.name = "PerspectiveCamera";
    createInfo.position = { 0.0F, 0.0F, 0.0F };
    createInfo.rotation = { 1.0F, 0.0F, 0.0F, 0.0F };
    createInfo.perspective = { .fieldOfView = fov, .aspectRatio = aspectRatio };

    return PerspectiveCamera{ createInfo };
}

void PerspectiveCamera::onTransformChanged() noexcept {}

void PerspectiveCamera::onProjectionChanged() noexcept
{
    markProjectionDirty();
}

void PerspectiveCamera::updateProjectionMatrix() const noexcept
{
    const f32 FOV_RADIANS = math::radians(m_config.fieldOfView);

    if (m_config.infiniteFarPlane) {
        m_projectionMatrix = math::perspectiveInfinite(
            FOV_RADIANS,
            m_config.aspectRatio,
            m_config.nearPlane
        );
    } else {
        m_projectionMatrix = math::perspective(
            FOV_RADIANS,
            m_config.aspectRatio,
            m_config.nearPlane,
            m_config.farPlane
        );
    }

    m_projectionMatrixDirty = false;
}

auto PerspectiveCamera::validateConfig(const PerspectiveConfig &config) noexcept
    -> std::expected<bool, std::string>
{
    if (config.fieldOfView <= 0.0F || config.fieldOfView >= 180.0F) {
        return std::unexpected(
            std::format(
                "Invalid field of view: {}. Must be in (0, 180) degrees.",
                config.fieldOfView
            )
        );
    }

    if (config.aspectRatio <= 0.0F) {
        return std::unexpected(
            std::format(
                "Invalid aspect ratio: {}. Must be greater than zero.",
                config.aspectRatio
            )
        );
    }

    if (config.nearPlane <= 0.0F) {
        return std::unexpected(
            std::format(
                "Invalid near plane: {}. Must be greater than zero.",
                config.nearPlane
            )
        );
    }

    if (!config.infiniteFarPlane && config.nearPlane >= config.farPlane) {
        return std::unexpected(
            std::format(
                "Near plane ({}) must be less than far plane ({}).",
                config.nearPlane,
                config.farPlane
            )
        );
    }

    if (!config.infiniteFarPlane && config.farPlane <= config.nearPlane) {
        return std::unexpected(
            std::format(
                "Far plane ({}) must be greater than near plane ({}).",
                config.farPlane,
                config.nearPlane
            )
        );
    }

    return {};
}

void PerspectiveCamera::markProjectionDirty() noexcept
{
    m_projectionMatrixDirty = true;
}

} // namespace vostok::graphics