#include "vostok/core/logger/logger.hpp"
#include "vostok/core/type.hpp"
#include "vostok/graphics/camera/perspective_camera.hpp"
#include "vostok/math/types.hpp"

#include <cmath>
#include <gtest/gtest.h>

using namespace vostok::graphics;
using namespace vostok;

class PerspectiveCameraTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        [[maybe_unused]] auto loggerResult = vostok::Logger::init();
    }

    static auto createDefaultCamera() -> PerspectiveCamera
    {
        PerspectiveCamera::PerspectiveConfig config{ .fieldOfView = 60.0F,
                                                     .aspectRatio =
                                                         16.0F / 9.0F,
                                                     .nearPlane = 0.1F,
                                                     .farPlane = 100.0F,
                                                     .infiniteFarPlane =
                                                         false };

        PerspectiveCamera::CreateInfo createInfo;
        createInfo.name = "TestCamera";
        createInfo.position = { 0.0F, 0.0F, 0.0F };
        createInfo.rotation = { 1.0F, 0.0F, 0.0F, 0.0F };
        createInfo.perspective = config;

        return PerspectiveCamera(createInfo);
    }
};

TEST_F(PerspectiveCameraTest, DefaultConstruction)
{
    auto camera = createDefaultCamera();

    EXPECT_FLOAT_EQ(camera.getFieldOfView(), 60.0F);
    EXPECT_FLOAT_EQ(camera.getAspectRatio(), 16.0F / 9.0F);
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.1F);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 100.0F);
    EXPECT_FALSE(camera.isInfiniteFarPlane());
    EXPECT_EQ(camera.getCameraType(), CameraType::PERSPECTIVE);
    EXPECT_EQ(camera.getName(), "TestCamera");
}

TEST_F(PerspectiveCameraTest, StaticCreateDefault)
{
    auto camera = PerspectiveCamera::createDefault();

    EXPECT_GT(camera.getFieldOfView(), 0.0F);
    EXPECT_GT(camera.getAspectRatio(), 0.0F);
    EXPECT_GT(camera.getNearPlane(), 0.0F);
    EXPECT_GT(camera.getFarPlane(), camera.getNearPlane());
}

TEST_F(PerspectiveCameraTest, StaticCreateWithFOV)
{
    const f32 TEST_FOV = 75.0F;
    const f32 TEST_ASPECT_RATIO = 4.0F / 3.0F;
    auto camera = PerspectiveCamera::createWithFOV(TEST_FOV, TEST_ASPECT_RATIO);

    EXPECT_FLOAT_EQ(camera.getFieldOfView(), TEST_FOV);
    EXPECT_FLOAT_EQ(camera.getAspectRatio(), TEST_ASPECT_RATIO);
}

TEST_F(PerspectiveCameraTest, SetFieldOfView_ValidValues)
{
    auto camera = createDefaultCamera();

    auto result1 = camera.setFieldOfView(45.0F);
    EXPECT_TRUE(result1.has_value());
    EXPECT_FLOAT_EQ(camera.getFieldOfView(), 45.0F);

    auto result2 = camera.setFieldOfView(120.0F);
    EXPECT_TRUE(result2.has_value());
    EXPECT_FLOAT_EQ(camera.getFieldOfView(), 120.0F);

    auto result3 = camera.setFieldOfView(0.1F);
    EXPECT_TRUE(result3.has_value());
    EXPECT_FLOAT_EQ(camera.getFieldOfView(), 0.1F);

    auto result4 = camera.setFieldOfView(179.9F);
    EXPECT_TRUE(result4.has_value());
    EXPECT_FLOAT_EQ(camera.getFieldOfView(), 179.9F);
}

TEST_F(PerspectiveCameraTest, SetFieldOfView_InvalidValues)
{
    auto camera = createDefaultCamera();
    const f32 ORIGINAL_FOV = camera.getFieldOfView();

    auto result1 = camera.setFieldOfView(0.0F);
    EXPECT_FALSE(result1.has_value());
    EXPECT_FLOAT_EQ(camera.getFieldOfView(), ORIGINAL_FOV);

    auto result2 = camera.setFieldOfView(-10.0F);
    EXPECT_FALSE(result2.has_value());
    EXPECT_FLOAT_EQ(camera.getFieldOfView(), ORIGINAL_FOV);

    auto result3 = camera.setFieldOfView(180.0F);
    EXPECT_FALSE(result3.has_value());
    EXPECT_FLOAT_EQ(camera.getFieldOfView(), ORIGINAL_FOV);

    auto result4 = camera.setFieldOfView(200.0F);
    EXPECT_FALSE(result4.has_value());
    EXPECT_FLOAT_EQ(camera.getFieldOfView(), ORIGINAL_FOV);
}

TEST_F(PerspectiveCameraTest, SetAspectRatio_ValidValues)
{
    auto camera = createDefaultCamera();

    auto result1 = camera.setAspectRatio(1.0F);
    EXPECT_TRUE(result1.has_value());
    EXPECT_FLOAT_EQ(camera.getAspectRatio(), 1.0F);

    auto result2 = camera.setAspectRatio(2.35F);
    EXPECT_TRUE(result2.has_value());
    EXPECT_FLOAT_EQ(camera.getAspectRatio(), 2.35F);

    auto result3 = camera.setAspectRatio(0.5F);
    EXPECT_TRUE(result3.has_value());
    EXPECT_FLOAT_EQ(camera.getAspectRatio(), 0.5F);
}

TEST_F(PerspectiveCameraTest, SetAspectRatio_InvalidValues)
{
    auto camera = createDefaultCamera();
    const f32 ORIGINAL_ASPECT = camera.getAspectRatio();

    auto result1 = camera.setAspectRatio(0.0F);
    EXPECT_FALSE(result1.has_value());
    EXPECT_FLOAT_EQ(camera.getAspectRatio(), ORIGINAL_ASPECT);

    auto result2 = camera.setAspectRatio(-1.0F);
    EXPECT_FALSE(result2.has_value());
    EXPECT_FLOAT_EQ(camera.getAspectRatio(), ORIGINAL_ASPECT);
}

TEST_F(PerspectiveCameraTest, SetNearPlane_ValidValues)
{
    auto camera = createDefaultCamera();

    auto result1 = camera.setNearPlane(0.01F);
    EXPECT_TRUE(result1.has_value());
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.01F);

    auto result2 = camera.setNearPlane(1.0F);
    EXPECT_TRUE(result2.has_value());
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 1.0F);
}

TEST_F(PerspectiveCameraTest, SetNearPlane_InvalidValues)
{
    auto camera = createDefaultCamera();
    const f32 ORIGINAL_NEAR = camera.getNearPlane();

    auto result1 = camera.setNearPlane(0.0F);
    EXPECT_FALSE(result1.has_value());
    EXPECT_FLOAT_EQ(camera.getNearPlane(), ORIGINAL_NEAR);

    auto result2 = camera.setNearPlane(-1.0F);
    EXPECT_FALSE(result2.has_value());
    EXPECT_FLOAT_EQ(camera.getNearPlane(), ORIGINAL_NEAR);

    auto result3 = camera.setNearPlane(camera.getFarPlane());
    EXPECT_FALSE(result3.has_value());
    EXPECT_FLOAT_EQ(camera.getNearPlane(), ORIGINAL_NEAR);
}

TEST_F(PerspectiveCameraTest, SetFarPlane_ValidValues)
{
    auto camera = createDefaultCamera();

    auto result1 = camera.setFarPlane(1000.0F);
    EXPECT_TRUE(result1.has_value());
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 1000.0F);

    auto result2 = camera.setFarPlane(10.0F);
    EXPECT_TRUE(result2.has_value());
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 10.0F);
}

TEST_F(PerspectiveCameraTest, SetFarPlane_InvalidValues)
{
    auto camera = createDefaultCamera();
    const f32 ORIGINAL_FAR = camera.getFarPlane();

    auto result1 = camera.setFarPlane(0.0F);
    EXPECT_FALSE(result1.has_value());
    EXPECT_FLOAT_EQ(camera.getFarPlane(), ORIGINAL_FAR);

    auto result2 = camera.setFarPlane(-10.0F);
    EXPECT_FALSE(result2.has_value());
    EXPECT_FLOAT_EQ(camera.getFarPlane(), ORIGINAL_FAR);

    auto result3 = camera.setFarPlane(camera.getNearPlane());
    EXPECT_FALSE(result3.has_value());
    EXPECT_FLOAT_EQ(camera.getFarPlane(), ORIGINAL_FAR);
}

TEST_F(PerspectiveCameraTest, SetInfiniteFarPlane)
{
    auto camera = createDefaultCamera();

    EXPECT_FALSE(camera.isInfiniteFarPlane());

    camera.setInfiniteFarPlane(true);
    EXPECT_TRUE(camera.isInfiniteFarPlane());

    camera.setInfiniteFarPlane(false);
    EXPECT_FALSE(camera.isInfiniteFarPlane());
}

TEST_F(PerspectiveCameraTest, UpdateConfig_ValidConfig)
{
    auto camera = createDefaultCamera();

    PerspectiveCamera::PerspectiveConfig newConfig{ .fieldOfView = 90.0F,
                                                    .aspectRatio = 1.0F,
                                                    .nearPlane = 0.5F,
                                                    .farPlane = 500.0F,
                                                    .infiniteFarPlane = true };

    auto result = camera.updateConfig(newConfig);
    EXPECT_TRUE(result.has_value());

    EXPECT_FLOAT_EQ(camera.getFieldOfView(), 90.0F);
    EXPECT_FLOAT_EQ(camera.getAspectRatio(), 1.0F);
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.5F);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 500.0F);
    EXPECT_TRUE(camera.isInfiniteFarPlane());
}

TEST_F(PerspectiveCameraTest, UpdateConfig_InvalidConfig)
{
    auto camera = createDefaultCamera();
    const f32 ORIGINAL_FOV = camera.getFieldOfView();

    PerspectiveCamera::PerspectiveConfig invalidConfig{ .fieldOfView = -45.0F,
                                                        .aspectRatio = 1.0F,
                                                        .nearPlane = 0.1F,
                                                        .farPlane = 100.0F,
                                                        .infiniteFarPlane =
                                                            false };

    auto result = camera.updateConfig(invalidConfig);
    EXPECT_FALSE(result.has_value());

    EXPECT_FLOAT_EQ(camera.getFieldOfView(), ORIGINAL_FOV);
}

TEST_F(PerspectiveCameraTest, ProjectionMatrix_NotNull)
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

TEST_F(PerspectiveCameraTest, ViewMatrix_NotNull)
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

TEST_F(PerspectiveCameraTest, ViewProjectionMatrix)
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

TEST_F(PerspectiveCameraTest, FrustumCorners)
{
    auto camera = createDefaultCamera();

    auto corners = camera.getFrustumCorners();

    EXPECT_EQ(corners.size(), 8);

    for (size_t i = 0; i < corners.size(); ++i) {
        for (size_t j = i + 1; j < corners.size(); ++j) {
            bool isDifferent = (corners[i].x != corners[j].x) ||
                               (corners[i].y != corners[j].y) ||
                               (corners[i].z != corners[j].z);
            EXPECT_TRUE(isDifferent);
        }
    }
}

TEST_F(PerspectiveCameraTest, WorldToScreenRay)
{
    auto camera = createDefaultCamera();

    math::Vec2 screenPos{ 0.5F, 0.5F };
    math::Vec2 screenSize{ 1920.0F, 1080.0F };

    auto [rayOrigin, rayDirection] =
        camera.getWorldToScreenRay(screenPos, screenSize);
    f32 dirLength = std::sqrt(
        (rayDirection.x * rayDirection.x) + (rayDirection.y * rayDirection.y) +
        (rayDirection.z * rayDirection.z)
    );
    EXPECT_NEAR(dirLength, 1.0F, 0.1F);
}

TEST_F(PerspectiveCameraTest, CameraBaseFunctionality_Position)
{
    auto camera = createDefaultCamera();

    math::Vec3 newPos{ 1.0F, 2.0F, 3.0F };
    camera.setPosition(newPos);

    EXPECT_FLOAT_EQ(camera.getPosition().x, 1.0F);
    EXPECT_FLOAT_EQ(camera.getPosition().y, 2.0F);
    EXPECT_FLOAT_EQ(camera.getPosition().z, 3.0F);
}

TEST_F(PerspectiveCameraTest, CameraBaseFunctionality_Name)
{
    auto camera = createDefaultCamera();

    camera.setName("NewCameraName");
    EXPECT_EQ(camera.getName(), "NewCameraName");
}

TEST_F(PerspectiveCameraTest, CameraBaseFunctionality_Translation)
{
    auto camera = createDefaultCamera();

    math::Vec3 originalPos = camera.getPosition();
    math::Vec3 delta{ 1.0F, 0.0F, 0.0F };

    camera.translate(delta);

    EXPECT_FLOAT_EQ(camera.getPosition().x, originalPos.x + delta.x);
    EXPECT_FLOAT_EQ(camera.getPosition().y, originalPos.y + delta.y);
    EXPECT_FLOAT_EQ(camera.getPosition().z, originalPos.z + delta.z);
}

TEST_F(PerspectiveCameraTest, CameraBaseFunctionality_Movement)
{
    auto camera = createDefaultCamera();

    math::Vec3 originalPos = camera.getPosition();

    EXPECT_NO_THROW(camera.moveForward(1.0F));
    EXPECT_NO_THROW(camera.moveRight(1.0F));
    EXPECT_NO_THROW(camera.moveUp(1.0F));
}

TEST_F(PerspectiveCameraTest, CameraBaseFunctionality_DirectionVectors)
{
    auto camera = createDefaultCamera();

    auto forward = camera.getForward();
    auto right = camera.getRight();
    auto up = camera.getUp();
    f32 forwardLength = std::sqrt(
        (forward.x * forward.x) + (forward.y * forward.y) +
        (forward.z * forward.z)
    );
    f32 rightLength = std::sqrt(
        (right.x * right.x) + (right.y * right.y) + (right.z * right.z)
    );
    f32 upLength = std::sqrt((up.x * up.x) + (up.y * up.y) + (up.z * up.z));

    EXPECT_NEAR(forwardLength, 1.0F, 0.1F);
    EXPECT_NEAR(rightLength, 1.0F, 0.1F);
    EXPECT_NEAR(upLength, 1.0F, 0.1F);
}

TEST_F(PerspectiveCameraTest, CameraBaseFunctionality_LookAt)
{
    auto camera = createDefaultCamera();

    LookAtParams params;
    params.target = { 0.0F, 0.0F, -5.0F };
    params.up = { 0.0F, 1.0F, 0.0F };

    EXPECT_NO_THROW(camera.lookAt(params));
}

TEST(OrthographicBasicTest, GoogleTestSanity)
{
    EXPECT_EQ(1 + 1, 2);
    SUCCEED();
}
