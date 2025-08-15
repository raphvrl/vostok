#pragma once

#include "vostok/graphics/buffers/buffer.hpp"

#include <expected>
#include <memory>
#include <span>
#include <string>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanInstance;
class VulkanDevice;
class VulkanAllocator;
class VulkanFrameSync;

class VulkanBuffer : public graphics::Buffer
{
public:
    static auto create(
        VulkanInstance *instance,
        VulkanDevice *device,
        VulkanAllocator *allocator,
        VulkanFrameSync *frameSync,
        const graphics::BufferCreateInfo &info
    ) -> std::expected<std::unique_ptr<VulkanBuffer>, std::string>;

    ~VulkanBuffer() override;

    VulkanBuffer(const VulkanBuffer &) = delete;
    auto operator=(const VulkanBuffer &) -> VulkanBuffer & = delete;
    VulkanBuffer(VulkanBuffer &&) noexcept;
    auto operator=(VulkanBuffer &&) noexcept -> VulkanBuffer &;

    [[nodiscard]] auto getBuffer() const -> VkBuffer;
    [[nodiscard]] auto getAllocation() const -> VmaAllocation;

    void bind() override;

    auto map() -> std::expected<std::span<std::byte>, std::string> override;
    void unmap() override;

    auto update(std::span<const std::byte> data, size_t offset = 0)
        -> std::expected<void, std::string> override;

    auto copyFrom(
        const graphics::Buffer &source,
        size_t srcOffset = 0,
        size_t dstOffset = 0,
        size_t size = 0
    ) -> std::expected<void, std::string> override;

    [[nodiscard]] auto getSize() const -> size_t override;
    [[nodiscard]] auto getUsage() const -> graphics::BufferUsage override;
    [[nodiscard]] auto getMemory() const -> graphics::BufferMemory override;

    [[nodiscard]] auto getOffset() const -> size_t override;
    auto setOffset(size_t offset) -> void override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    VulkanBuffer(std::unique_ptr<Impl> impl);

    friend class VulkanGPU;
};

} // namespace vostok::graphics::vulkan
