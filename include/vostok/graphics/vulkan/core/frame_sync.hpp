#pragma once

#include "vostok/core/type.hpp"

#include <expected>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

struct FrameData
{
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkFence inFlight = VK_NULL_HANDLE;
};

class Device;

class FrameSync
{
public:
    struct CreateInfo
    {
        Device *device = nullptr;
        u32 maxFramesInFlight = 2;
        VkCommandPool commandPool = VK_NULL_HANDLE;
    };

    ~FrameSync();

    FrameSync(const FrameSync &) = delete;
    FrameSync &operator=(const FrameSync &) = delete;
    FrameSync(FrameSync &&other) noexcept;
    FrameSync &operator=(FrameSync &&other) noexcept;

    static std::expected<std::unique_ptr<FrameSync>, std::string>
    create(const CreateInfo &createInfo);

    [[nodiscard]] VkSemaphore getImageAvailableSemaphore() const;
    [[nodiscard]] VkSemaphore getRenderFinishedSemaphore() const;
    [[nodiscard]] VkFence getInFlightFence() const;
    [[nodiscard]] VkCommandBuffer getCommandBuffer() const;

    void waitForFence();
    void resetFences();
    void nextFrame();

    std::expected<void, std::string> beginCommandBuffer();
    std::expected<void, std::string> endCommandBuffer();

private:
    FrameSync(Device *device, VkCommandPool commandPool, u32 maxFramesInFlight);
    bool init();

    Device *m_device = nullptr;
    u32 m_maxFramesInFlight = 2;
    u32 m_currentFrame = 0;

    std::vector<FrameData> m_frames;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan