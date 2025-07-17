#pragma once

#include "vostok/graphics/vulkan/core/device.hpp"
#include "vostok/graphics/vulkan/core/instance.hpp"
#include "vostok/graphics/vulkan/core/physical_device.hpp"

#include <expected>
#include <memory>
#include <string>
#include <vk_mem_alloc.h>

namespace vostok::graphics::vulkan
{

class Allocator
{
public:
    struct CreateInfo
    {
        Instance *instance = nullptr;
        PhysicalDevice *physicalDevice = nullptr;
        Device *device = nullptr;
        u32 vulkanApiVersion = VK_API_VERSION_1_4;
    };

    static auto create(const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<Allocator>, std::string>;

    ~Allocator();

    Allocator(const Allocator &) = delete;
    auto operator=(const Allocator &) -> Allocator & = delete;
    Allocator(Allocator &&other) noexcept;
    auto operator=(Allocator &&other) noexcept -> Allocator &;

    auto mapMemory(VmaAllocation allocation, void **data)
        -> std::expected<void *, std::string>;
    void unmapMemory(VmaAllocation allocation);

    auto createBuffer(
        const VkBufferCreateInfo &createInfo,
        VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY
    ) -> std::expected<std::pair<VkBuffer, VmaAllocation>, std::string>;

    void destroyBuffer(VkBuffer buffer, VmaAllocation allocation);

    [[nodiscard]] auto getHandle() const -> VmaAllocator { return m_allocator; }

private:
    Allocator(VmaAllocator allocator);

    VmaAllocator m_allocator = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan