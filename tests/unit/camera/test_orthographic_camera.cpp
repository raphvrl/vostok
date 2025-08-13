#include "vostok/core/logger/logger.hpp"
#include "vostok/graphics/camera/orthographic_camera.hpp"
#include "vostok/math/types.hpp"

#include <cmath>
#include <gtest/gtest.h>

using namespace vostok::graphics;
using namespace vostok;

class OrthographicCameraTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        [[maybe_unused]] auto loggerResult = vostok::Logger::init();
    }

    static auto createDefaultCamera() -> OrthographicCamerahandle
    {
        OrthographicCamerahandle::OrthographicConfig config{ .left = -1.0F,
                                                             .right = 1.0F,
                                                             .bottom = -1.0F,
                                                             .top = 1.0F,
                                                             .nearPlane = 0.1F,
                                                             .farPlane =
                                                                 100.0F };
        OrthographicCamerahandle::CreateInfo createInfo;
        createInfo.name = "TestOrthoCamera";
        createInfo.position = { 0.0F, 0.0F, 0.0F };
        createInfo.rotation = { 1.0F, 0.0F, 0.0F, 0.0F };
        createInfo.config = config;
        return OrthographicCamerahandle(createInfo);
    }
};

TEST_F(OrthographicCameraTest, DefaultConstruction)
{
    auto camera = createDefaultCamera();
    EXPECT_FLOAT_EQ(camera.getLeft(), -1.0F);
    EXPECT_FLOAT_EQ(camera.getRight(), 1.0F);
    EXPECT_FLOAT_EQ(camera.getBottom(), -1.0F);
    EXPECT_FLOAT_EQ(camera.getTop(), 1.0F);
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.1F);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 100.0F);
    EXPECT_EQ(camera.getCameraType(), CameraType::ORTHOGRAPHIC);
    EXPECT_EQ(camera.getName(), "TestOrthoCamera");
}

TEST_F(OrthographicCameraTest, StaticCreateCentered)
{
    CenteredParams params{ .width = 4.0F,
                           .height = 2.0F,
                           .nearPlane = 0.1F,
                           .farPlane = 10.0F };
    auto camera = OrthographicCamerahandle::createCentered(params);
    EXPECT_FLOAT_EQ(camera.getLeft(), -2.0F);
    EXPECT_FLOAT_EQ(camera.getRight(), 2.0F);
    EXPECT_FLOAT_EQ(camera.getBottom(), -1.0F);
    EXPECT_FLOAT_EQ(camera.getTop(), 1.0F);
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.1F);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 10.0F);
}

TEST_F(OrthographicCameraTest, StaticCreateUI)
{
    float w = 1920.0F;
    float h = 1080.0F;
    auto camera = OrthographicCamerahandle::createUI(w, h);
    EXPECT_FLOAT_EQ(camera.getLeft(), 0.0F);
    EXPECT_FLOAT_EQ(camera.getRight(), w);
    EXPECT_FLOAT_EQ(camera.getBottom(), 0.0F);
    EXPECT_FLOAT_EQ(camera.getTop(), h);
    EXPECT_FLOAT_EQ(camera.getNearPlane(), -1.0F);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 1.0F);
}

TEST_F(OrthographicCameraTest, StaticCreateFromBounds)
{
    BoundsParams params{ .left = -10.0F,
                         .right = 10.0F,
                         .bottom = -5.0F,
                         .top = 5.0F,
                         .nearPlane = 0.5F,
                         .farPlane = 50.0F };
    auto camera = OrthographicCamerahandle::createFromBounds(params);
    EXPECT_FLOAT_EQ(camera.getLeft(), -10.0F);
    EXPECT_FLOAT_EQ(camera.getRight(), 10.0F);
    EXPECT_FLOAT_EQ(camera.getBottom(), -5.0F);
    EXPECT_FLOAT_EQ(camera.getTop(), 5.0F);
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.5F);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 50.0F);
}

TEST_F(OrthographicCameraTest, SetBounds_Valid)
{
    auto camera = createDefaultCamera();
    auto result = camera.setBounds(-2.0F, 2.0F, -1.0F, 1.0F);
    EXPECT_TRUE(result.has_value());
    EXPECT_FLOAT_EQ(camera.getLeft(), -2.0F);
    EXPECT_FLOAT_EQ(camera.getRight(), 2.0F);
    EXPECT_FLOAT_EQ(camera.getBottom(), -1.0F);
    EXPECT_FLOAT_EQ(camera.getTop(), 1.0F);
}

TEST_F(OrthographicCameraTest, SetBounds_Invalid)
{
    auto camera = createDefaultCamera();
    auto result1 = camera.setBounds(2.0F, -2.0F, -1.0F, 1.0F);
    EXPECT_FALSE(result1.has_value());
    auto result2 = camera.setBounds(-2.0F, 2.0F, 1.0F, -1.0F);
    EXPECT_FALSE(result2.has_value());
}

TEST_F(OrthographicCameraTest, SetPlanes_Valid)
{
    auto camera = createDefaultCamera();
    auto result = camera.setPlanes(0.5F, 10.0F);
    EXPECT_TRUE(result.has_value());
    EXPECT_FLOAT_EQ(camera.getNearPlane(), 0.5F);
    EXPECT_FLOAT_EQ(camera.getFarPlane(), 10.0F);
}

TEST_F(OrthographicCameraTest, SetPlanes_Invalid)
{
    auto camera = createDefaultCamera();
    auto result = camera.setPlanes(10.0F, 0.5F);
    EXPECT_FALSE(result.has_value());
}

TEST_F(OrthographicCameraTest, UpdateConfig_Valid)
{
    auto camera = createDefaultCamera();
    OrthographicCamerahandle::OrthographicConfig config{ .left = -5.0F,
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

TEST_F(OrthographicCameraTest, UpdateConfig_Invalid)
{
    auto camera = createDefaultCamera();
    OrthographicCamerahandle::OrthographicConfig config{ .left = 5.0F,
                                                         .right = -5.0F,
                                                         .bottom = -2.0F,
                                                         .top = 2.0F,
                                                         .nearPlane = 0.2F,
                                                         .farPlane = 20.0F };
    auto result = camera.updateConfig(config);
    EXPECT_FALSE(result.has_value());
}

TEST_F(OrthographicCameraTest, ProjectionMatrix_NotNull)
{
    auto camera = createDefaultCamera();
    const auto &projMatrix = camera.getProjectionMatrix();
    bool isIdentity = true;
    for (int i = 0; i < 4 && isIdentity; ++i) {
        for (int j = 0; j < 4 && isIdentity; ++j) {
            float expected = (i == j) ? 1.0F : 0.0F;
            if (std::abs(projMatrix[i][j] - expected) > 1e-6F) {
                isIdentity = false;
            }
        }
    }
    EXPECT_FALSE(isIdentity);
}

TEST_F(OrthographicCameraTest, ViewMatrix_NotNull)
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

TEST_F(OrthographicCameraTest, ViewProjectionMatrix)
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

TEST_F(OrthographicCameraTest, CameraBaseFunctionality_Position)
{
    auto camera = createDefaultCamera();
    math::Vec3 newPos{ 1.0F, 2.0F, 3.0F };
    camera.setPosition(newPos);
    EXPECT_FLOAT_EQ(camera.getPosition().x, 1.0F);
    EXPECT_FLOAT_EQ(camera.getPosition().y, 2.0F);
    EXPECT_FLOAT_EQ(camera.getPosition().z, 3.0F);
}

TEST_F(OrthographicCameraTest, CameraBaseFunctionality_Name)
{
    auto camera = createDefaultCamera();
    camera.setName("NewOrthoCameraName");
    EXPECT_EQ(camera.getName(), "NewOrthoCameraName");
}

TEST_F(OrthographicCameraTest, CameraBaseFunctionality_Translation)
{
    auto camera = createDefaultCamera();
    math::Vec3 originalPos = camera.getPosition();
    math::Vec3 delta{ 1.0F, 0.0F, 0.0F };
    camera.translate(delta);
    EXPECT_FLOAT_EQ(camera.getPosition().x, originalPos.x + delta.x);
    EXPECT_FLOAT_EQ(camera.getPosition().y, originalPos.y + delta.y);
    EXPECT_FLOAT_EQ(camera.getPosition().z, originalPos.z + delta.z);
}

TEST_F(OrthographicCameraTest, CameraBaseFunctionality_Movement)
{
    auto camera = createDefaultCamera();
    math::Vec3 originalPos = camera.getPosition();
    EXPECT_NO_THROW(camera.moveForward(1.0F));
    EXPECT_NO_THROW(camera.moveRight(1.0F));
    EXPECT_NO_THROW(camera.moveUp(1.0F));
}

TEST(BasicTest, GoogleTestSanity)
{
    EXPECT_EQ(1 + 1, 2);
    SUCCEED();
}