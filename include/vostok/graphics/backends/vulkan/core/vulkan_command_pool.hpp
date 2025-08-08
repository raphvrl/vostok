#pragma once

#include <expected>
#include <memory>
#include <span>
#include <string>
#include <vostok/core/type.hpp>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanDevice;

class VulkanCommandPool
{
public:
    struct CreateInfo
    {
        VulkanDevice *device;
        u32 queueFamilyIndex;
        VkCommandPoolCreateFlags flags;
        u32 maxFramesInFlight;
    };

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<VulkanCommandPool>, std::string>;

    ~VulkanCommandPool();

    VulkanCommandPool(const VulkanCommandPool &) = delete;
    auto operator=(const VulkanCommandPool &) -> VulkanCommandPool & = delete;
    VulkanCommandPool(VulkanCommandPool &&) noexcept;
    auto operator=(VulkanCommandPool &&) noexcept -> VulkanCommandPool &;

    [[nodiscard]] auto getHandle() const -> VkCommandPool { return m_pool; }

    auto allocate(VkCommandBufferLevel level)
        -> std::expected<VkCommandBuffer, std::string>;

    void free(VkCommandBuffer buffer);
    void free(std::span<VkCommandBuffer> buffers);

    void reset() const;

private:
    VulkanCommandPool(VulkanDevice *device, VkCommandPool pool);

    VulkanDevice *m_device;
    VkCommandPool m_pool;
};

} // namespace vostok::graphics::vulkan