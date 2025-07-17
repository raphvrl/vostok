#include "graphics/camera/orthographic_camera.hpp"

#include "core/logger/logger.hpp"
#include "math/projections.hpp"

#include <format>

namespace vostok::graphics
{

OrthographicCamera::OrthographicCamera(const CreateInfo &createInfo)
    : Camera(createInfo),
      m_config(createInfo.config)
{
    if (auto result = validateConfig(m_config); !result.has_value()) {
        Logger::error(
            "OrthographicCamera '{}' created with invalid config: {}",
            getName(),
            result.error()
        );
        Logger::warning(
            "Orthographic camera '{}' falling back to default configuration",
            getName()
        );
        m_config = OrthographicConfig{};
    } else {
        Logger::info(
            "OrthographicCamera '{}' created - Bounds: [{:.2f}, {:.2f}, "
            "{:.2f}, {:.2f}], Near: "
            "{:.3f}, Far: {:.1f}",
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

auto OrthographicCamera::getProjectionMatrix() const noexcept
    -> const math::Mat4 &
{
    if (m_projectionMatrixDirty) {
        updateProjectionMatrix();
    }

    return m_projectionMatrix;
}

auto OrthographicCamera::getViewMatrix() const noexcept -> const math::Mat4 &
{
    return Camera::getViewMatrix();
}

auto OrthographicCamera::getViewProjectionMatrix() const -> math::Mat4
{
    return getProjectionMatrix() * getViewMatrix();
}

auto OrthographicCamera::setBounds(
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
            "OrthographicCamera '{}' bounds changed: [{:.2f}, {:.2f}, {:.2f}, "
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

auto OrthographicCamera::setPlanes(f32 nearPlane, f32 farPlane) noexcept
    -> std::expected<void, std::string>
{
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
            "OrthographicCamera '{}' planes changed: Near {:.3f} -> {:.3f}, "
            "Far {:.1f} -> {:.1f}",
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

auto OrthographicCamera::updateConfig(const OrthographicConfig &config) noexcept
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
        m_config = config;
        onProjectionChanged();
    }

    return {};
}

auto OrthographicCamera::getAspectRatio() const noexcept -> f32
{
    const f32 WIDTH = getWidth();
    const f32 HEIGHT = getHeight();

    if (HEIGHT == 0.0F) [[unlikely]] {
        return 1.0F;
    }

    return WIDTH / HEIGHT;
}

auto OrthographicCamera::createCentered(const CenteredParams &params)
    -> OrthographicCamera
{
    const f32 HALF_WIDTH = params.width * 0.5F;
    const f32 HALF_HEIGHT = params.height * 0.5F;

    CreateInfo createInfo{};
    createInfo.name = "OrthographicCamera";
    createInfo.position = { 0.0F, 0.0F, 0.0F };
    createInfo.rotation = { 1.0F, 0.0F, 0.0F, 0.0F };
    createInfo.config = { .left = -HALF_WIDTH,
                          .right = HALF_WIDTH,
                          .bottom = -HALF_HEIGHT,
                          .top = HALF_HEIGHT,
                          .nearPlane = params.nearPlane,
                          .farPlane = params.farPlane };

    return OrthographicCamera{ createInfo };
}

auto OrthographicCamera::createUI(f32 screenWidth, f32 screenHeight)
    -> OrthographicCamera
{
    CreateInfo createInfo{};
    createInfo.name = "OrthographicCamera_UI";
    createInfo.position = { 0.0F, 0.0F, 0.0F };
    createInfo.rotation = { 1.0F, 0.0F, 0.0F, 0.0F };
    createInfo.config = { .left = 0.0F,
                          .right = screenWidth,
                          .bottom = 0.0F,
                          .top = screenHeight,
                          .nearPlane = -1.0F,
                          .farPlane = 1.0F };

    return OrthographicCamera{ createInfo };
}

auto OrthographicCamera::createFromBounds(const BoundsParams &params)
    -> OrthographicCamera
{
    CreateInfo createInfo{};
    createInfo.name = "OrthographicCamera";
    createInfo.position = { 0.0F, 0.0F, 0.0F };
    createInfo.rotation = { 1.0F, 0.0F, 0.0F, 0.0F };
    createInfo.config = { .left = params.left,
                          .right = params.right,
                          .bottom = params.bottom,
                          .top = params.top,
                          .nearPlane = params.nearPlane,
                          .farPlane = params.farPlane };

    return OrthographicCamera{ createInfo };
}

void OrthographicCamera::onTransformChanged() noexcept {}

void OrthographicCamera::onProjectionChanged() noexcept
{
    markProjectionDirty();
}

void OrthographicCamera::updateProjectionMatrix() const noexcept
{
    Logger::trace(
        "OrthographicCamera '{}' updating projection matrix",
        getName()
    );

    m_projectionMatrix = math::ortho(
        m_config.left,
        m_config.right,
        m_config.bottom,
        m_config.top,
        m_config.nearPlane,
        m_config.farPlane
    );

    m_projectionMatrixDirty = false;
}

auto OrthographicCamera::validateConfig(
    const OrthographicConfig &config
) noexcept -> std::expected<bool, std::string>
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

    if (config.nearPlane >= config.farPlane) {
        return std::unexpected(
            std::format(
                "Near plane ({}) must be less than far plane ({}).",
                config.nearPlane,
                config.farPlane
            )
        );
    }

    return true;
}

void OrthographicCamera::markProjectionDirty() noexcept
{
    m_projectionMatrixDirty = true;
}

} // namespace vostok::graphics