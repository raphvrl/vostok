#pragma once

#include "vostok/core/type.hpp"

#include <expected>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanDevice;
class VulkanCommandPool;

class VulkanFrameSync
{
public:
    struct FrameData
    {
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;
        VkFence inFlight = VK_NULL_HANDLE;
    };

    struct CommandPools
    {
        VulkanCommandPool *graphics = nullptr;
        VulkanCommandPool *transfer = nullptr;
    };

    struct CreateInfo
    {
        VulkanDevice *device = nullptr;
        u32 maxFramesInFlight = 2;
        CommandPools commandPools;
        VkQueue transferQueue = VK_NULL_HANDLE;
    };

    ~VulkanFrameSync();

    VulkanFrameSync(const VulkanFrameSync &) = delete;
    auto operator=(const VulkanFrameSync &) -> VulkanFrameSync & = delete;
    VulkanFrameSync(VulkanFrameSync &&other) noexcept;
    auto operator=(VulkanFrameSync &&other) noexcept -> VulkanFrameSync &;

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<VulkanFrameSync>, std::string>;

    [[nodiscard]] auto getImageAvailableSemaphore() const -> VkSemaphore;
    [[nodiscard]] auto getRenderFinishedSemaphore() const -> VkSemaphore;
    [[nodiscard]] auto getInFlightFence() const -> VkFence;
    [[nodiscard]] auto getCommandBuffer() const -> VkCommandBuffer;

    void waitForFence();
    void resetFences();
    void nextFrame();

    auto beginCommandBuffer() -> std::expected<void, std::string>;
    auto endCommandBuffer() -> std::expected<void, std::string>;

    void cmdDraw(
        u32 vertexCount,
        u32 instanceCount = 1,
        u32 firstVertex = 0,
        u32 firstInstance = 0
    );

    [[nodiscard]] auto getTransferCommandBuffer() const -> VkCommandBuffer;
    auto beginTransferCommandBuffer() -> std::expected<void, std::string>;
    auto endTransferCommandBuffer() -> std::expected<void, std::string>;
    auto submitTransferCommandBuffer() -> std::expected<void, std::string>;
    auto waitForTransferComplete() -> std::expected<void, std::string>;

    [[nodiscard]] auto getTransferCommandPool() const -> VulkanCommandPool *
    {
        return m_transferCommandPool;
    }

private:
    VulkanFrameSync(
        VulkanDevice *device,
        const CommandPools &commandPools,
        VkQueue transferQueue,
        u32 maxFramesInFlight
    );
    auto init() -> bool;

    VulkanDevice *m_device = nullptr;
    u32 m_maxFramesInFlight = 2;
    u32 m_currentFrame = 0;

    std::vector<FrameData> m_frames;

    VulkanCommandPool *m_graphicsCommandPool = nullptr;

    VulkanCommandPool *m_transferCommandPool = nullptr;
    VkQueue m_transferQueue = VK_NULL_HANDLE;
    VkCommandBuffer m_transferCommandBuffer = VK_NULL_HANDLE;
    VkFence m_transferFence = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan