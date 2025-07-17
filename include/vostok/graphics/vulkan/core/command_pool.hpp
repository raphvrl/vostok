#pragma once

#include <expected>
#include <memory>
#include <span>
#include <string>
#include <vostok/core/type.hpp>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class Device;

class CommandPool
{
public:
    struct CreateInfo
    {
        Device *device;
        u32 queueFamilyIndex;
        VkCommandPoolCreateFlags flags;
        u32 maxFramesInFlight;
    };

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<CommandPool>, std::string>;

    ~CommandPool();

    CommandPool(const CommandPool &) = delete;
    auto operator=(const CommandPool &) -> CommandPool & = delete;
    CommandPool(CommandPool &&) noexcept;
    auto operator=(CommandPool &&) noexcept -> CommandPool &;

    [[nodiscard]] auto getHandle() const -> VkCommandPool { return m_pool; }

    auto allocate(VkCommandBufferLevel level)
        -> std::expected<VkCommandBuffer, std::string>;

    void free(VkCommandBuffer buffer);
    void free(std::span<VkCommandBuffer> buffers);

    void reset() const;

private:
    CommandPool(Device *device, VkCommandPool pool);

    Device *m_device;
    VkCommandPool m_pool;
};

} // namespace vostok::graphics::vulkan