#pragma once

#include "vostok/graphics/camera/frustum_camera.hpp"
#include "vostok/graphics/camera/orthographic_camera.hpp"
#include "vostok/graphics/camera/perspective_camera.hpp"

#include <memory>

namespace vostok::graphics
{

using PerspectiveCameraPtr = std::shared_ptr<PerspectiveCamera>;
using OrthographicCameraPtr = std::shared_ptr<OrthographicCamera>;
using FrustumCameraPtr = std::shared_ptr<FrustumCamera>;

} // namespace vostok::graphics