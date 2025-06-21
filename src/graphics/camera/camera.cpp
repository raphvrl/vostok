#include "graphics/camera/camera.hpp"

#include "core/logger/logger.hpp"
#include "math/functions.hpp"

namespace vostok::graphics
{

Camera::Camera(const CreateInfo &createInfo)
    : m_name(createInfo.name),
      m_position(createInfo.position),
      m_rotation(createInfo.rotation)
{
    Logger::info(
        "Camera '{}' created at position {{:.2f}, {:.2f}, {:.2f}}",
        m_name,
        m_position.x,
        m_position.y,
        m_position.z
    );

    markDirty();
}

auto Camera::setPosition(const math::Vec3 &position) noexcept -> void
{
    if (m_position == position) [[likely]] {
        return;
    }

    const auto DISTANCE = math::distance(m_position, position);
    if (DISTANCE > 0.1F) {
        Logger::trace(
            "Camera '{}' moved from position {{:.2f}, {:.2f}, {:.2f}} to {{:.2f}, {:.2f}, {:.2f}}",
            m_name,
            m_position.x,
            m_position.y,
            m_position.z,
            position.x,
            position.y,
            position.z
        );
    }

    m_position = position;
    onTransformChanged();
    markDirty();
}

auto Camera::setRotation(const math::Quat &rotation) noexcept -> void
{
    if (m_rotation == rotation) [[likely]] {
        return;
    }

    const auto ROTATION_DIFF = rotation * glm::conjugate(m_rotation);
    const auto ANGLE_DIFF = math::angle(ROTATION_DIFF);

    if (ANGLE_DIFF > 0.1F) {
        Logger::trace("Camera '{}' rotation changed by {:.1f}", m_name, math::degrees(ANGLE_DIFF));
    }

    m_rotation = rotation;
    onTransformChanged();
    markDirty();
}

auto Camera::setName(const std::string &name) noexcept -> void
{
    if (m_name != name) {
        Logger::info("Camera renamed: '{}' -> '{}'", m_name, name);
    }
    m_name = name;
}

auto Camera::getForward() const noexcept -> math::Vec3
{
    return math::normalize(m_rotation * math::Vec3{ 0.0F, 0.0F, -1.0F });
}

auto Camera::getRight() const noexcept -> math::Vec3
{
    return math::normalize(m_rotation * math::Vec3{ 1.0F, 0.0F, 0.0F });
}

auto Camera::getUp() const noexcept -> math::Vec3
{
    return math::normalize(m_rotation * WORLD_UP);
}

auto Camera::translate(const math::Vec3 &delta) noexcept -> void
{
    setPosition(m_position + delta);
}

auto Camera::rotate(const math::Quat &delta) noexcept -> void
{
    setRotation(m_rotation * delta);
}

auto Camera::lookAt(const LookAtParams &params) noexcept -> void
{
    Logger::debug(
        "Camera '{}' looking at target {{:.2f}, {:.2f}, {:.2f}}",
        m_name,
        params.target.x,
        params.target.y,
        params.target.z,
        params.up.x,
        params.up.y,
        params.up.z
    );

    const auto DIRECTION = math::normalize(params.target - m_position);
    setRotation(math::quatLookAt(DIRECTION, params.up));
}

auto Camera::moveForward(f32 distance) noexcept -> void
{
    translate(getForward() * distance);
}

auto Camera::moveRight(f32 distance) noexcept -> void
{
    translate(getRight() * distance);
}

auto Camera::moveUp(f32 distance) noexcept -> void
{
    translate(getUp() * distance);
}

auto Camera::getViewMatrix() const noexcept -> const math::Mat4 &
{
    if (m_viewMatrixDirty) [[unlikely]] {
        updateViewMatrix();
    }
    return m_viewMatrix;
}

auto Camera::updateViewMatrix() const noexcept -> void
{
    static thread_local size_t s_updateCount = 0;
    if (s_updateCount % 60 == 0) {
        Logger::trace("Camera '{}' view matrix updated {} times", m_name, s_updateCount);
    }

    const auto ROTATION_MATRIX = math::convertQuaternionToMat4(math::conjugate(m_rotation));
    const auto TRANSLATION_MATRIX = glm::translate(math::Mat4{ 1.0F }, -m_position);

    m_viewMatrix = ROTATION_MATRIX * TRANSLATION_MATRIX;
    m_viewMatrixDirty = false;
}

auto Camera::markDirty() noexcept -> void
{
    m_viewMatrixDirty = true;
    m_projectionMatrixDirty = true;
}

} // namespace vostok::graphics