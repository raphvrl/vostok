#include "vostok/core/logger/logger.hpp"
#include "vostok/graphics/camera/frustum_camera.hpp"
#include "vostok/graphics/camera/perspective_camera.hpp"
#include "vostok/math/types.hpp"

#include <cmath>
#include <gtest/gtest.h>

using namespace vostok::graphics;
using namespace vostok;

class FrustumCameraTest : public ::testing::Test
{
protected:
    void SetUp() override { [[maybe_unused]] auto loggerResult = vostok::Logger::init(); }

    static auto createDefaultCamera() -> FrustumCamera
    {
        FrustumCamera::FrustumConfig config{ .left = -1.0F,
                                             .right = 1.0F,
                                             .bottom = -1.0F,
                                             .top = 1.0F,
                                             .nearPlane = 0.1F,
                                             .farPlane = 100.0F };

        FrustumCamera::CreateInfo createInfo;
        createInfo.name = "TestFrustumCamera";
        createInfo.position = { 0.0F, 0.0F, 0.0F };
        createInfo.rotation = { 1.0F, 0.0F, 0.0F, 0.0F };
        createInfo.config = config;

        return FrustumCamera(createInfo);
    }

    static auto createTestPerspectiveCamera() -> PerspectiveCamera
    {
        PerspectiveCamera::PerspectiveConfig config{ .fieldOfView = 60.0F,
                                                     .aspectRatio = 16.0F / 9.0F,
                                                     .nearPlane = 0.1F,
                                                     .farPlane = 100.0F,
                                                     .infiniteFarPlane = false };

        PerspectiveCamera::CreateInfo createInfo;
        createInfo.name = "TestPerspectiveCamera";
        createInfo.position = { 0.0F, 0.0F, 0.0F };
        createInfo.rotation = { 1.0F, 0.0F, 0.0F, 0.0F };
        createInfo.perspective = config;

        return PerspectiveCamera(createInfo);
    }
};

TEST_F(FrustumCameraTest, DefaultConstruction)
{
    auto camera = createDefaultCamera();

    EXPECT_FLOAT_EQ(camera.getLeft(), -1.0F);
    EXPECT_FLOAT_EQ(camera.getRight(), 1.0F);
    EXPECT_FLOAT_EQ(camera.getBottom(), -1.0F);
    EXPECT_FLOAT_EQ(camera.getTop(), 1.0F);
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.1F);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 100.0F);
    EXPECT_EQ(camera.getCameraType(), CameraType::FRUSTUM);
    EXPECT_EQ(camera.getName(), "TestFrustumCamera");
}

TEST_F(FrustumCameraTest, StaticCreateFromPerspective)
{
    auto perspectiveCamera = createTestPerspectiveCamera();
    auto frustumCamera = FrustumCamera::createFromPerspective(perspectiveCamera);

    EXPECT_EQ(frustumCamera.getCameraType(), CameraType::FRUSTUM);
    EXPECT_GT(frustumCamera.getNearPlane(), 0.0F);
    EXPECT_GT(frustumCamera.getFarPlane(), frustumCamera.getNearPlane());
    EXPECT_LT(frustumCamera.getLeft(), frustumCamera.getRight());
    EXPECT_LT(frustumCamera.getBottom(), frustumCamera.getTop());

    EXPECT_FLOAT_EQ(frustumCamera.getNearPlane(), perspectiveCamera.getNearPlane());
    EXPECT_FLOAT_EQ(frustumCamera.getFarPlane(), perspectiveCamera.getFarPlane());
}

TEST_F(FrustumCameraTest, SetBounds_Valid)
{
    auto camera = createDefaultCamera();

    auto result = camera.setBounds(-2.0F, 2.0F, -1.0F, 1.0F);
    EXPECT_TRUE(result.has_value());

    EXPECT_FLOAT_EQ(camera.getLeft(), -2.0F);
    EXPECT_FLOAT_EQ(camera.getRight(), 2.0F);
    EXPECT_FLOAT_EQ(camera.getBottom(), -1.0F);
    EXPECT_FLOAT_EQ(camera.getTop(), 1.0F);
}

TEST_F(FrustumCameraTest, SetBounds_Invalid)
{
    auto camera = createDefaultCamera();
    const f32 ORIGINAL_LEFT = camera.getLeft();

    auto result1 = camera.setBounds(2.0F, -2.0F, -1.0F, 1.0F);
    EXPECT_FALSE(result1.has_value());
    EXPECT_FLOAT_EQ(camera.getLeft(), ORIGINAL_LEFT);

    auto result2 = camera.setBounds(-2.0F, 2.0F, 1.0F, -1.0F);
    EXPECT_FALSE(result2.has_value());
    EXPECT_FLOAT_EQ(camera.getLeft(), ORIGINAL_LEFT);
}

TEST_F(FrustumCameraTest, SetPlanes_Valid)
{
    auto camera = createDefaultCamera();

    auto result = camera.setPlanes(0.5F, 10.0F);
    EXPECT_TRUE(result.has_value());

    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.5F);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 10.0F);
}

TEST_F(FrustumCameraTest, SetPlanes_Invalid)
{
    auto camera = createDefaultCamera();
    const f32 ORIGINAL_NEAR = camera.getNearPlane();

    auto result1 = camera.setPlanes(0.0F, 10.0F);
    EXPECT_FALSE(result1.has_value());
    EXPECT_FLOAT_EQ(camera.getNearPlane(), ORIGINAL_NEAR);

    auto result2 = camera.setPlanes(-1.0F, 10.0F);
    EXPECT_FALSE(result2.has_value());
    EXPECT_FLOAT_EQ(camera.getNearPlane(), ORIGINAL_NEAR);

    auto result3 = camera.setPlanes(10.0F, 0.5F);
    EXPECT_FALSE(result3.has_value());
    EXPECT_FLOAT_EQ(camera.getNearPlane(), ORIGINAL_NEAR);
}

TEST_F(FrustumCameraTest, UpdateConfig_Valid)
{
    auto camera = createDefaultCamera();

    FrustumCamera::FrustumConfig config{ .left = -5.0F,
                                         .right = 5.0F,
                                         .bottom = -2.0F,
                                         .top = 2.0F,
                                         .nearPlane = 0.2F,
                                         .farPlane = 20.0F };

    auto result = camera.updateConfig(config);
    EXPECT_TRUE(result.has_value());

    EXPECT_FLOAT_EQ(camera.getLeft(), -5.0F);
    EXPECT_FLOAT_EQ(camera.getRight(), 5.0F);
    EXPECT_FLOAT_EQ(camera.getBottom(), -2.0F);
    EXPECT_FLOAT_EQ(camera.getTop(), 2.0F);
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.2F);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 20.0F);
}

TEST_F(FrustumCameraTest, UpdateConfig_Invalid)
{
    auto camera = createDefaultCamera();
    const f32 ORIGINAL_LEFT = camera.getLeft();

    FrustumCamera::FrustumConfig invalidConfig{ .left = 5.0F,
                                                .right = -5.0F,
                                                .bottom = -2.0F,
                                                .top = 2.0F,
                                                .nearPlane = 0.2F,
                                                .farPlane = 20.0F };

    auto result = camera.updateConfig(invalidConfig);
    EXPECT_FALSE(result.has_value());

    EXPECT_FLOAT_EQ(camera.getLeft(), ORIGINAL_LEFT);
}

TEST_F(FrustumCameraTest, ProjectionMatrix_NotNull)
{
    auto camera = createDefaultCamera();

    const auto &projMatrix = camera.getProjectionMatrix();

    bool isIdentity = true;
    for (int i = 0; i < 4 && isIdentity; ++i) {
        for (int j = 0; j < 4 && isIdentity; ++j) {
            f32 expected = (i == j) ? 1.0F : 0.0F;
            if (std::abs(projMatrix[i][j] - expected) > 1e-6F) {
                isIdentity = false;
            }
        }
    }
    EXPECT_FALSE(isIdentity);
}

TEST_F(FrustumCameraTest, ViewMatrix_NotNull)
{
    auto camera = createDefaultCamera();

    const auto &viewMatrix = camera.getViewMatrix();

    bool hasNonZeroValues = false;
    for (int i = 0; i < 4 && !hasNonZeroValues; ++i) {
        for (int j = 0; j < 4 && !hasNonZeroValues; ++j) {
            if (std::abs(viewMatrix[i][j]) > 1e-6F) {
                hasNonZeroValues = true;
            }
        }
    }
    EXPECT_TRUE(hasNonZeroValues);
}

TEST_F(FrustumCameraTest, ViewProjectionMatrix)
{
    auto camera = createDefaultCamera();

    auto vpMatrix = camera.getViewProjectionMatrix();

    bool hasNonZeroValues = false;
    for (int i = 0; i < 4 && !hasNonZeroValues; ++i) {
        for (int j = 0; j < 4 && !hasNonZeroValues; ++j) {
            if (std::abs(vpMatrix[i][j]) > 1e-6F) {
                hasNonZeroValues = true;
            }
        }
    }
    EXPECT_TRUE(hasNonZeroValues);
}

TEST_F(FrustumCameraTest, CameraBaseFunctionality_Position)
{
    auto camera = createDefaultCamera();

    math::Vec3 newPos{ 1.0F, 2.0F, 3.0F };
    camera.setPosition(newPos);

    EXPECT_FLOAT_EQ(camera.getPosition().x, 1.0F);
    EXPECT_FLOAT_EQ(camera.getPosition().y, 2.0F);
    EXPECT_FLOAT_EQ(camera.getPosition().z, 3.0F);
}

TEST_F(FrustumCameraTest, CameraBaseFunctionality_Name)
{
    auto camera = createDefaultCamera();

    camera.setName("NewFrustumCameraName");
    EXPECT_EQ(camera.getName(), "NewFrustumCameraName");
}

TEST_F(FrustumCameraTest, CameraBaseFunctionality_Translation)
{
    auto camera = createDefaultCamera();

    math::Vec3 originalPos = camera.getPosition();
    math::Vec3 delta{ 1.0F, 0.0F, 0.0F };

    camera.translate(delta);

    EXPECT_FLOAT_EQ(camera.getPosition().x, originalPos.x + delta.x);
    EXPECT_FLOAT_EQ(camera.getPosition().y, originalPos.y + delta.y);
    EXPECT_FLOAT_EQ(camera.getPosition().z, originalPos.z + delta.z);
}

TEST_F(FrustumCameraTest, CameraBaseFunctionality_Movement)
{
    auto camera = createDefaultCamera();

    math::Vec3 originalPos = camera.getPosition();

    EXPECT_NO_THROW(camera.moveForward(1.0F));
    EXPECT_NO_THROW(camera.moveRight(1.0F));
    EXPECT_NO_THROW(camera.moveUp(1.0F));
}

TEST_F(FrustumCameraTest, CameraBaseFunctionality_DirectionVectors)
{
    auto camera = createDefaultCamera();

    math::Vec3 forward = camera.Camera::getForward();
    math::Vec3 right = camera.Camera::getRight();
    math::Vec3 up = camera.Camera::getUp();

    f32 forwardLength =
        std::sqrt((forward.x * forward.x) + (forward.y * forward.y) + (forward.z * forward.z));
    f32 rightLength = std::sqrt((right.x * right.x) + (right.y * right.y) + (right.z * right.z));
    f32 upLength = std::sqrt((up.x * up.x) + (up.y * up.y) + (up.z * up.z));

    EXPECT_NEAR(forwardLength, 1.0F, 0.1F);
    EXPECT_NEAR(rightLength, 1.0F, 0.1F);
    EXPECT_NEAR(upLength, 1.0F, 0.1F);
}

TEST_F(FrustumCameraTest, CameraBaseFunctionality_LookAt)
{
    auto camera = createDefaultCamera();

    LookAtParams params;
    params.target = { 0.0F, 0.0F, -5.0F };
    params.up = { 0.0F, 1.0F, 0.0F };

    EXPECT_NO_THROW(camera.lookAt(params));
}

TEST_F(FrustumCameraTest, ConversionFromPerspective_Accuracy)
{
    auto perspectiveCamera = createTestPerspectiveCamera();
    auto frustumCamera = FrustumCamera::createFromPerspective(perspectiveCamera);

    const auto &perspectiveMatrix = perspectiveCamera.getProjectionMatrix();
    const auto &frustumMatrix = frustumCamera.getProjectionMatrix();

    bool matricesAreSimilar = true;
    for (int i = 0; i < 4 && matricesAreSimilar; ++i) {
        for (int j = 0; j < 4 && matricesAreSimilar; ++j) {
            if (std::abs(perspectiveMatrix[i][j] - frustumMatrix[i][j]) > 0.1F) {
                if (std::abs(perspectiveMatrix[i][j]) > 0.01F &&
                    std::abs(frustumMatrix[i][j]) > 0.01F) {
                    matricesAreSimilar = false;
                }
            }
        }
    }
    EXPECT_TRUE(matricesAreSimilar);
}

TEST(FrustumBasicTest, GoogleTestSanity)
{
    EXPECT_EQ(1 + 1, 2);
    SUCCEED();
}