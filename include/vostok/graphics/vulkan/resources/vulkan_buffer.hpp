#pragma once

#include "vostok/graphics/buffer.hpp"

#include <expected>
#include <memory>
#include <span>
#include <string>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanAllocator;
class VulkanGPUDevice;

class VulkanBuffer : public graphics::Buffer
{
public:
    ~VulkanBuffer() override;

    VulkanBuffer(const VulkanBuffer &) = delete;
    auto operator=(const VulkanBuffer &) -> VulkanBuffer & = delete;
    VulkanBuffer(VulkanBuffer &&) noexcept;
    auto operator=(VulkanBuffer &&) noexcept -> VulkanBuffer &;

    [[nodiscard]] auto getBuffer() const -> VkBuffer;
    [[nodiscard]] auto getAllocation() const -> VmaAllocation;

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

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    VulkanBuffer(std::unique_ptr<Impl> impl);

    friend class VulkanGPUDevice;

    static auto
    create(VulkanGPUDevice *device, const graphics::BufferCreateInfo &info)
        -> std::unique_ptr<VulkanBuffer>;
};

} // namespace vostok::graphics::vulkan
