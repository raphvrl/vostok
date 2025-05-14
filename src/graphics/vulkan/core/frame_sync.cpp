#include "graphics/vulkan/core/frame_sync.hpp"

#include "core/logger/logger.hpp"
#include "graphics/vulkan/core/device.hpp"
#include "volk.h"

#include <expected>

namespace vostok::graphics::vulkan
{

FrameSync::FrameSync(Device *device, VkCommandPool commandPool, u32 maxFramesInFlight)
    : m_device(device),
      m_maxFramesInFlight(maxFramesInFlight),
      m_commandPool(commandPool)
{}

FrameSync::~FrameSync()
{
    for (auto &frame : m_frames) {
        if (frame.commandBuffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(m_device->getHandle(), m_commandPool, 1, &frame.commandBuffer);
        }

        if (frame.imageAvailable != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device->getHandle(), frame.imageAvailable, nullptr);
        }

        if (frame.renderFinished != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device->getHandle(), frame.renderFinished, nullptr);
        }

        if (frame.inFlight != VK_NULL_HANDLE) {
            vkDestroyFence(m_device->getHandle(), frame.inFlight, nullptr);
        }
    }

    m_frames.clear();
}

bool FrameSync::init()
{
    Logger::debug("Command pool created");

    m_frames.resize(m_maxFramesInFlight);

    VkCommandBufferAllocateInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferInfo.commandPool = m_commandPool;
    commandBufferInfo.commandBufferCount = 1;

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < m_maxFramesInFlight; ++i) {
        VkResult result = vkAllocateCommandBuffers(
            m_device->getHandle(),
            &commandBufferInfo,
            &m_frames[i].commandBuffer
        );

        if (result != VK_SUCCESS) {
            Logger::error("Failed to allocate command buffer");
            return false;
        }

        result = vkCreateSemaphore(
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

        result = vkCreateFence(m_device->getHandle(), &fenceInfo, nullptr, &m_frames[i].inFlight);

        if (result != VK_SUCCESS) {
            Logger::error("Failed to create in-flight fence");
            return false;
        }
    }

    return true;
}

std::expected<std::unique_ptr<FrameSync>, std::string>
FrameSync::create(const CreateInfo &createInfo)
{
    auto frameSync = std::unique_ptr<FrameSync>(
        new FrameSync(createInfo.device, createInfo.commandPool, createInfo.maxFramesInFlight)
    );

    if (!frameSync->init()) {
        return std::unexpected("Failed to create frame synchronization objects");
    }

    Logger::info("Frame synchronization objects created");

    return frameSync;
}

void FrameSync::waitForFence()
{
    vkWaitForFences(
        m_device->getHandle(),
        1,
        &m_frames[m_currentFrame].inFlight,
        VK_TRUE,
        UINT64_MAX
    );
}

void FrameSync::resetFences()
{
    vkResetFences(m_device->getHandle(), 1, &m_frames[m_currentFrame].inFlight);
}

void FrameSync::nextFrame()
{
    m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
}

VkSemaphore FrameSync::getImageAvailableSemaphore() const
{
    return m_frames[m_currentFrame].imageAvailable;
}

VkSemaphore FrameSync::getRenderFinishedSemaphore() const
{
    return m_frames[m_currentFrame].renderFinished;
}

VkFence FrameSync::getInFlightFence() const
{
    return m_frames[m_currentFrame].inFlight;
}

VkCommandBuffer FrameSync::getCommandBuffer() const
{
    return m_frames[m_currentFrame].commandBuffer;
}

std::expected<void, std::string> FrameSync::beginCommandBuffer()
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult result = vkBeginCommandBuffer(m_frames[m_currentFrame].commandBuffer, &beginInfo);

    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to begin command buffer");
    }

    return {};
}

std::expected<void, std::string> FrameSync::endCommandBuffer()
{
    VkResult result = vkEndCommandBuffer(m_frames[m_currentFrame].commandBuffer);

    if (result != VK_SUCCESS) {
        return std::unexpected("Failed to end command buffer");
    }

    return {};
}

} // namespace vostok::graphics::vulkan