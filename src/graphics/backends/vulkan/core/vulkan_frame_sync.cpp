#include "graphics/backends/vulkan/core/vulkan_frame_sync.hpp"

#include "core/logger/logger.hpp"
#include "graphics/backends/vulkan/core/vulkan_command_pool.hpp"
#include "graphics/backends/vulkan/core/vulkan_device.hpp"
#include "graphics/backends/vulkan/utils/vk_utils.hpp"
#include "volk.h"

#include <expected>

namespace vostok::graphics::vulkan
{

VulkanFrameSync::VulkanFrameSync(
    VulkanDevice *device,
    const CommandPools &commandPools,
    VkQueue transferQueue,
    u32 maxFramesInFlight
)
    : m_device(device),
      m_maxFramesInFlight(maxFramesInFlight),
      m_graphicsCommandPool(commandPools.graphics),
      m_transferCommandPool(commandPools.transfer),
      m_transferQueue(transferQueue)
{}

VulkanFrameSync::~VulkanFrameSync()
{
    Logger::debug("FrameSync destructor called");

    if (m_device != nullptr) {
        VkDevice device = m_device->getHandle();

        m_device->waitIdle();

        if (m_transferCommandBuffer != VK_NULL_HANDLE &&
            m_transferCommandPool != nullptr) {
            Logger::trace("Freeing transfer command buffer");
            m_transferCommandPool->free(m_transferCommandBuffer);
            m_transferCommandBuffer = VK_NULL_HANDLE;
        }

        if (m_transferFence != VK_NULL_HANDLE) {
            Logger::debug("Destroying transfer fence");
            vkDestroyFence(device, m_transferFence, nullptr);
            m_transferFence = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < m_frames.size(); ++i) {
            auto &frame = m_frames[i];
            if (frame.commandBuffer != VK_NULL_HANDLE &&
                m_graphicsCommandPool != nullptr) {
                Logger::trace("Freeing graphics command buffer {}", i);
                m_graphicsCommandPool->free(frame.commandBuffer);
                frame.commandBuffer = VK_NULL_HANDLE;
            }

            if (frame.imageAvailable != VK_NULL_HANDLE) {
                Logger::debug("Destroying image available semaphore {}", i);
                vkDestroySemaphore(device, frame.imageAvailable, nullptr);
                frame.imageAvailable = VK_NULL_HANDLE;
            }

            if (frame.renderFinished != VK_NULL_HANDLE) {
                Logger::debug("Destroying render finished semaphore {}", i);
                vkDestroySemaphore(device, frame.renderFinished, nullptr);
                frame.renderFinished = VK_NULL_HANDLE;
            }

            if (frame.inFlight != VK_NULL_HANDLE) {
                Logger::debug("Destroying in-flight fence {}", i);
                vkDestroyFence(device, frame.inFlight, nullptr);
                frame.inFlight = VK_NULL_HANDLE;
            }
        }
    } else {
        Logger::warning("FrameSync destructor called with null device");
    }

    m_frames.clear();
    Logger::debug("FrameSync destructor completed");
}

auto VulkanFrameSync::init() -> bool
{
    Logger::debug("Initializing frame synchronization objects");
    m_frames.resize(m_maxFramesInFlight);

    if (m_graphicsCommandPool == nullptr) {
        Logger::error("Graphics command pool is null");
        return false;
    }

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_maxFramesInFlight; ++i) {
        auto commandBufferResult =
            m_graphicsCommandPool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        if (!commandBufferResult) {
            Logger::error(
                "Failed to allocate command buffer: {}",
                commandBufferResult.error()
            );
            return false;
        }
        m_frames[i].commandBuffer = commandBufferResult.value();

        VkResult result = vkCreateSemaphore(
            m_device->getHandle(),
            &semaphoreInfo,
            nullptr,
            &m_frames[i].imageAvailable
        );

        if (result != VK_SUCCESS) {
            Logger::error("Failed to create image available semaphore");
            return false;
        }

        result = vkCreateSemaphore(
            m_device->getHandle(),
            &semaphoreInfo,
            nullptr,
            &m_frames[i].renderFinished
        );

        if (result != VK_SUCCESS) {
            Logger::error("Failed to create render finished semaphore");
            return false;
        }

        result = vkCreateFence(
            m_device->getHandle(),
            &fenceInfo,
            nullptr,
            &m_frames[i].inFlight
        );

        if (result != VK_SUCCESS) {
            Logger::error("Failed to create in-flight fence");
            return false;
        }
    }

    if (m_transferCommandPool != nullptr) {
        auto transferCommandBufferResult =
            m_transferCommandPool->allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        if (!transferCommandBufferResult) {
            Logger::error(
                "Failed to allocate transfer command buffer: {}",
                transferCommandBufferResult.error()
            );
            return false;
        }
        m_transferCommandBuffer = transferCommandBufferResult.value();
    }

    VkFenceCreateInfo transferFenceInfo = {};
    transferFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    transferFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkResult result = vkCreateFence(
        m_device->getHandle(),
        &transferFenceInfo,
        nullptr,
        &m_transferFence
    );
    if (result != VK_SUCCESS) {
        Logger::error("Failed to create transfer fence");
        return false;
    }
    Logger::debug("Frame synchronization objects initialized successfully");
    return true;
}

auto VulkanFrameSync::create(const CreateInfo &createInfo)
    -> std::expected<std::unique_ptr<VulkanFrameSync>, std::string>
{
    auto frameSync = std::unique_ptr<VulkanFrameSync>(new VulkanFrameSync(
        createInfo.device,
        createInfo.commandPools,
        createInfo.transferQueue,
        createInfo.maxFramesInFlight
    ));

    if (!frameSync->init()) {
        return std::unexpected(
            "Failed to create frame synchronization objects"
        );
    }

    Logger::info("Frame synchronization objects created");

    return frameSync;
}

void VulkanFrameSync::waitForFence()
{
    constexpr u64 TIMEOUT_NS = 1000000000ULL;

    VkResult result = vkWaitForFences(
        m_device->getHandle(),
        1,
        &m_frames[m_currentFrame].inFlight,
        VK_TRUE,
        TIMEOUT_NS
    );

    if (result == VK_TIMEOUT) {
        Logger::warning("Frame fence wait timed out after 1 second");
    } else if (result != VK_SUCCESS) {
        Logger::error(
            "Failed to wait for frame fence: {}",
            utils::vkResultToString(result)
        );
    }
}

void VulkanFrameSync::resetFences()
{
    vkResetFences(m_device->getHandle(), 1, &m_frames[m_currentFrame].inFlight);
}

void VulkanFrameSync::nextFrame()
{
    m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
}

auto VulkanFrameSync::getImageAvailableSemaphore() const -> VkSemaphore
{
    return m_frames[m_currentFrame].imageAvailable;
}

auto VulkanFrameSync::getRenderFinishedSemaphore() const -> VkSemaphore
{
    return m_frames[m_currentFrame].renderFinished;
}

auto VulkanFrameSync::getInFlightFence() const -> VkFence
{
    return m_frames[m_currentFrame].inFlight;
}

auto VulkanFrameSync::getCommandBuffer() const -> VkCommandBuffer
{
    return m_frames[m_currentFrame].commandBuffer;
}

auto VulkanFrameSync::beginCommandBuffer() -> std::expected<void, std::string>
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult result = vkBeginCommandBuffer(
        m_frames[m_currentFrame].commandBuffer,
        &beginInfo
    );

    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to begin command buffer");
    }

    return {};
}

auto VulkanFrameSync::endCommandBuffer() -> std::expected<void, std::string>
{
    VkResult result =
        vkEndCommandBuffer(m_frames[m_currentFrame].commandBuffer);

    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to end command buffer");
    }

    return {};
}

auto VulkanFrameSync::submitCommandBuffer() -> std::expected<void, std::string>
{
    if (m_frames.empty()) {
        return std::unexpected("No frames available");
    }

    auto &frame = m_frames[m_currentFrame];
    if (frame.commandBuffer == VK_NULL_HANDLE) {
        return std::unexpected("No command buffer to submit");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frame.commandBuffer;

    VkResult result = vkQueueSubmit(
        m_device->getGraphicsQueue(),
        1,
        &submitInfo,
        frame.inFlight
    );

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to submit graphics command buffer: " +
            utils::vkResultToString(result)
        );
    }

    return {};
}

auto VulkanFrameSync::waitForComplete() -> std::expected<void, std::string>
{
    if (m_frames.empty()) {
        return std::unexpected("No frames available");
    }

    auto &frame = m_frames[m_currentFrame];

    VkResult result = vkWaitForFences(
        m_device->getHandle(),
        1,
        &frame.inFlight,
        VK_TRUE,
        1000000000ULL
    );

    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to wait for graphics command buffer completion: " +
            utils::vkResultToString(result)
        );
    }

    return {};
}

void VulkanFrameSync::cmdDraw(
    u32 vertexCount,
    u32 instanceCount,
    u32 firstVertex,
    u32 firstInstance
)
{
    vkCmdDraw(
        m_frames[m_currentFrame].commandBuffer,
        vertexCount,
        instanceCount,
        firstVertex,
        firstInstance
    );
}

void VulkanFrameSync::cmdDrawIndexed(
    u32 indexCount,
    u32 instanceCount,
    u32 firstIndex,
    u32 vertexOffset,
    u32 firstInstance
)
{
    vkCmdDrawIndexed(
        m_frames[m_currentFrame].commandBuffer,
        indexCount,
        instanceCount,
        firstIndex,
        static_cast<int>(vertexOffset),
        firstInstance
    );
}

auto VulkanFrameSync::getTransferCommandBuffer() const -> VkCommandBuffer
{
    return m_transferCommandBuffer;
}

auto VulkanFrameSync::beginTransferCommandBuffer()
    -> std::expected<void, std::string>
{
    VkResult result = vkWaitForFences(
        m_device->getHandle(),
        1,
        &m_transferFence,
        VK_TRUE,
        UINT64_MAX
    );
    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to wait for transfer fence");
    }

    result = vkResetFences(m_device->getHandle(), 1, &m_transferFence);
    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to reset transfer fence");
    }

    if (m_transferCommandPool != nullptr) {
        m_transferCommandPool->reset();
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(m_transferCommandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to begin transfer command buffer");
    }

    Logger::trace("Transfer command buffer recording started");

    return {};
}

auto VulkanFrameSync::endTransferCommandBuffer()
    -> std::expected<void, std::string>
{
    VkResult result = vkEndCommandBuffer(m_transferCommandBuffer);
    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to end transfer command buffer");
    }
    Logger::trace("Transfer command buffer recording ended");
    return {};
}

auto VulkanFrameSync::submitTransferCommandBuffer()
    -> std::expected<void, std::string>
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_transferCommandBuffer;
    VkResult result =
        vkQueueSubmit(m_transferQueue, 1, &submitInfo, m_transferFence);

    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to submit transfer command buffer");
    }

    Logger::trace("Transfer command buffer submitted to queue");
    return {};
}

auto VulkanFrameSync::waitForTransferComplete()
    -> std::expected<void, std::string>
{
    constexpr u64 TIMEOUT_NS = 1000000000ULL;

    VkResult result = vkWaitForFences(
        m_device->getHandle(),
        1,
        &m_transferFence,
        VK_TRUE,
        TIMEOUT_NS
    );

    if (result == VK_TIMEOUT) {
        Logger::warning("Transfer operation timed out after 1 second");
        return std::unexpected("Transfer operation timed out");
    }

    if (result != VK_SUCCESS) {
        Logger::error(
            "Failed to wait for transfer completion: {}",
            utils::vkResultToString(result)
        );
        return std::unexpected("Failed to wait for transfer completion");
    }

    Logger::trace("Transfer operation completed");
    return {};
}

} // namespace vostok::graphics::vulkan