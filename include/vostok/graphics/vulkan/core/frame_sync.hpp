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
    auto operator=(const FrameSync &) -> FrameSync & = delete;
    FrameSync(FrameSync &&other) noexcept;
    auto operator=(FrameSync &&other) noexcept -> FrameSync &;

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<FrameSync>, std::string>;

    [[nodiscard]] auto getImageAvailableSemaphore() const -> VkSemaphore;
    [[nodiscard]] auto getRenderFinishedSemaphore() const -> VkSemaphore;
    [[nodiscard]] auto getInFlightFence() const -> VkFence;
    [[nodiscard]] auto getCommandBuffer() const -> VkCommandBuffer;

    void waitForFence();
    void resetFences();
    void nextFrame();

    auto beginCommandBuffer() -> std::expected<void, std::string>;
    auto endCommandBuffer() -> std::expected<void, std::string>;

    void
    cmdDraw(u32 vertexCount, u32 instanceCount = 1, u32 firstVertex = 0, u32 firstInstance = 0);

private:
    FrameSync(Device *device, VkCommandPool commandPool, u32 maxFramesInFlight);
    auto init() -> bool;

    Device *m_device = nullptr;
    u32 m_maxFramesInFlight = 2;
    u32 m_currentFrame = 0;

    std::vector<FrameData> m_frames;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan