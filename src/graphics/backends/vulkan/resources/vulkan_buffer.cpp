#include "vostok/graphics/backends/vulkan/resources/vulkan_buffer.hpp"

#include "vostok/core/logger/logger.hpp"
#include "vostok/graphics/backends/vulkan/core/vulkan_allocator.hpp"
#include "vostok/graphics/backends/vulkan/core/vulkan_frame_sync.hpp"
#include "vostok/graphics/backends/vulkan/utils/vk_utils.hpp"
#include "vostok/graphics/backends/vulkan/vulkan_gpu.hpp"

#include <cstring>
#include <vk_mem_alloc.h>
#include <volk.h>

namespace vostok::graphics::vulkan
{

class VulkanBuffer::Impl
{
public:
    Impl(
        VulkanAllocator *allocator,
        VulkanFrameSync *frameSync,
        VkQueue transferQueue,
        VkBuffer buffer,
        VmaAllocation allocation,
        graphics::BufferUsage usage,
        graphics::BufferMemory memory,
        size_t size
    );
    ~Impl();

    Impl(const Impl &) = delete;
    auto operator=(const Impl &) -> Impl & = delete;
    Impl(Impl &&other) noexcept;
    auto operator=(Impl &&other) noexcept -> Impl &;

    void bind();

    auto map() -> std::expected<std::span<std::byte>, std::string>;
    void unmap();

    auto update(std::span<const std::byte> data, size_t offset = 0)
        -> std::expected<void, std::string>;

    [[nodiscard]] auto getBuffer() const -> VkBuffer { return m_buffer; }
    [[nodiscard]] auto getAllocation() const -> VmaAllocation
    {
        return m_allocation;
    }
    [[nodiscard]] auto getSize() const -> size_t { return m_size; }
    [[nodiscard]] auto getUsage() const -> graphics::BufferUsage
    {
        return m_usage;
    }
    [[nodiscard]] auto getMemory() const -> graphics::BufferMemory
    {
        return m_memory;
    }

    auto copyFrom(
        const graphics::Buffer &source,
        size_t srcOffset,
        size_t dstOffset,
        size_t size
    ) -> std::expected<void, std::string>;

    [[nodiscard]] auto getOffset() const -> size_t { return m_offset; }
    auto setOffset(size_t offset) -> void { m_offset = offset; }

private:
    void destroyBuffer();

    auto updateGpuOnly(std::span<const std::byte> data, size_t offset = 0)
        -> std::expected<void, std::string>;

    auto updateCpuAccessible(std::span<const std::byte> data, size_t offset = 0)
        -> std::expected<void, std::string>;

    VulkanAllocator *m_allocator = nullptr;
    VulkanFrameSync *m_frameSync = nullptr;
    VkQueue m_transferQueue = VK_NULL_HANDLE;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    graphics::BufferUsage m_usage;
    graphics::BufferMemory m_memory;
    VkDeviceSize m_size;
    VkDeviceSize m_offset = 0;
    void *m_mapped = nullptr;
};

VulkanBuffer::Impl::Impl(
    VulkanAllocator *allocator,
    VulkanFrameSync *frameSync,
    VkQueue transferQueue,
    VkBuffer buffer,
    VmaAllocation allocation,
    graphics::BufferUsage usage,
    graphics::BufferMemory memory,
    size_t size
)
    : m_allocator(allocator),
      m_frameSync(frameSync),
      m_transferQueue(transferQueue),
      m_buffer(buffer),
      m_allocation(allocation),
      m_usage(usage),
      m_memory(memory),
      m_size(size)
{}

VulkanBuffer::Impl::~Impl()
{
    destroyBuffer();
}

VulkanBuffer::Impl::Impl(Impl &&other) noexcept
    : m_allocator(other.m_allocator),
      m_frameSync(other.m_frameSync),
      m_transferQueue(other.m_transferQueue),
      m_buffer(other.m_buffer),
      m_allocation(other.m_allocation),
      m_usage(other.m_usage),
      m_memory(other.m_memory),
      m_size(other.m_size),
      m_offset(other.m_offset),
      m_mapped(other.m_mapped)
{
    other.m_buffer = VK_NULL_HANDLE;
    other.m_allocation = VK_NULL_HANDLE;
    other.m_offset = 0;
    other.m_mapped = nullptr;
}

auto VulkanBuffer::Impl::operator=(Impl &&other) noexcept -> Impl &
{
    if (this != &other) {
        destroyBuffer();
        m_allocator = other.m_allocator;
        m_frameSync = other.m_frameSync;
        m_transferQueue = other.m_transferQueue;
        m_buffer = other.m_buffer;
        m_allocation = other.m_allocation;
        m_usage = other.m_usage;
        m_memory = other.m_memory;
        m_size = other.m_size;
        m_offset = other.m_offset;
        m_mapped = other.m_mapped;

        other.m_buffer = VK_NULL_HANDLE;
        other.m_allocation = VK_NULL_HANDLE;
        other.m_offset = 0;
        other.m_mapped = nullptr;
    }

    return *this;
}

void VulkanBuffer::Impl::bind()
{
    auto *commandBuffer = m_frameSync->getCommandBuffer();

    const u32 USAGE = static_cast<u32>(m_usage);
    if ((USAGE & static_cast<u32>(graphics::BufferUsage::VERTEX)) != 0U) {
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_buffer, &m_offset);
    }
    if ((USAGE & static_cast<u32>(graphics::BufferUsage::INDEX)) != 0U) {
        vkCmdBindIndexBuffer(
            commandBuffer,
            m_buffer,
            m_offset,
            VK_INDEX_TYPE_UINT32
        );
    }
}

auto VulkanBuffer::Impl::map()
    -> std::expected<std::span<std::byte>, std::string>
{
    if (m_mapped != nullptr) {
        return std::span<std::byte>(static_cast<std::byte *>(m_mapped), m_size);
    }

    void *data = nullptr;

    auto result = m_allocator->mapMemory(m_allocation, &data);
    if (!result) {
        return std::unexpected(result.error());
    }

    m_mapped = result.value();
    return std::span<std::byte>(static_cast<std::byte *>(m_mapped), m_size);
}

void VulkanBuffer::Impl::unmap()
{
    if (m_mapped == nullptr) {
        return;
    }

    m_allocator->unmapMemory(m_allocation);
    m_mapped = nullptr;
}

auto VulkanBuffer::Impl::update(std::span<const std::byte> data, size_t offset)
    -> std::expected<void, std::string>
{
    if (offset + data.size() > m_size) {
        return std::unexpected("Data size exceeds buffer size");
    }

    size_t requiredAlignment = utils::getRequiredAlignment(m_usage);

    if (offset % requiredAlignment != 0) {
        Logger::warning(
            "Update offset {} is not aligned to {} bytes for buffer usage {}. "
            "This may cause "
            "performance issues or validation errors.",
            offset,
            requiredAlignment,
            static_cast<int>(m_usage)
        );
    }

    if (m_memory == graphics::BufferMemory::GPU_ONLY) {
        return updateGpuOnly(data, offset);
    }

    return updateCpuAccessible(data, offset);
}

void VulkanBuffer::Impl::destroyBuffer()
{
    if (m_buffer != VK_NULL_HANDLE) {
        m_allocator->destroyBuffer(m_buffer, m_allocation);
    }
}

auto VulkanBuffer::Impl::updateGpuOnly(
    std::span<const std::byte> data,
    size_t offset
) -> std::expected<void, std::string>
{
    VkBufferCreateInfo stagingInfo = {};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = data.size();
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    auto stagingResult =
        m_allocator->createBuffer(stagingInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
    if (!stagingResult) {
        return std::unexpected(
            "Failed to create staging buffer: " + stagingResult.error()
        );
    }

    auto [stagingBuffer, stagingAllocation] = stagingResult.value();

    void *mapped = nullptr;
    auto mapResult = m_allocator->mapMemory(stagingAllocation, &mapped);
    if (!mapResult) {
        m_allocator->destroyBuffer(stagingBuffer, stagingAllocation);
        return std::unexpected(
            "Failed to map staging buffer: " + mapResult.error()
        );
    }

    std::memcpy(mapped, data.data(), data.size());
    m_allocator->unmapMemory(stagingAllocation);

    auto beginResult = m_frameSync->beginTransferCommandBuffer();
    if (!beginResult) {
        m_allocator->destroyBuffer(stagingBuffer, stagingAllocation);
        return std::unexpected(
            "Failed to begin transfer command buffer: " + beginResult.error()
        );
    }

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = offset;
    copyRegion.size = data.size();

    vkCmdCopyBuffer(
        m_frameSync->getTransferCommandBuffer(),
        stagingBuffer,
        m_buffer,
        1,
        &copyRegion
    );

    auto endResult = m_frameSync->endTransferCommandBuffer();
    if (!endResult) {
        m_allocator->destroyBuffer(stagingBuffer, stagingAllocation);
        return std::unexpected(
            "Failed to end transfer command buffer: " + endResult.error()
        );
    }

    auto submitResult = m_frameSync->submitTransferCommandBuffer();
    if (!submitResult) {
        m_allocator->destroyBuffer(stagingBuffer, stagingAllocation);
        return std::unexpected(
            "Failed to submit transfer command buffer: " + submitResult.error()
        );
    }

    auto waitResult = m_frameSync->waitForTransferComplete();
    if (!waitResult) {
        m_allocator->destroyBuffer(stagingBuffer, stagingAllocation);
        return std::unexpected(
            "Failed to wait for transfer completion: " + waitResult.error()
        );
    }

    m_allocator->destroyBuffer(stagingBuffer, stagingAllocation);
    return {};
}

auto VulkanBuffer::Impl::updateCpuAccessible(
    std::span<const std::byte> data,
    size_t offset
) -> std::expected<void, std::string>
{
    auto mapped = map();
    if (!mapped) {
        return std::unexpected(mapped.error());
    }

    std::memcpy(mapped->data() + offset, data.data(), data.size());
    unmap();
    return {};
}

auto VulkanBuffer::Impl::copyFrom(
    const graphics::Buffer &source,
    size_t srcOffset,
    size_t dstOffset,
    size_t size
) -> std::expected<void, std::string>
{
    if (srcOffset + size > source.getSize() || dstOffset + size > m_size) {
        return std::unexpected("Buffer copy size exceeds buffer size");
    }

    size_t requiredAlignment = utils::getRequiredAlignment(m_usage);

    if (srcOffset % requiredAlignment != 0 ||
        dstOffset % requiredAlignment != 0) {
        Logger::warning(
            "Buffer copy source offset {} or destination offset {} is not "
            "aligned to {} bytes for "
            "buffer usage {}. This may cause performance issues or validation "
            "errors.",
            srcOffset,
            dstOffset,
            requiredAlignment,
            static_cast<int>(m_usage)
        );
    }

    const auto *vulkanSource = dynamic_cast<const VulkanBuffer *>(&source);
    if (vulkanSource == nullptr) {
        return std::unexpected("Source buffer is not a Vulkan buffer");
    }

    if ((static_cast<u32>(source.getUsage()) &
         static_cast<u32>(graphics::BufferUsage::TRANSFER_SRC)) == 0U) {
        return std::unexpected(
            "Source buffer does not have TRANSFER_SRC usage flag"
        );
    }

    if ((static_cast<u32>(m_usage) &
         static_cast<u32>(graphics::BufferUsage::TRANSFER_DST)) == 0U) {
        return std::unexpected(
            "Destination buffer does not have TRANSFER_DST usage flag"
        );
    }

    auto *commandBuffer = m_frameSync->getTransferCommandBuffer();
    if (commandBuffer == VK_NULL_HANDLE) {
        return std::unexpected("Failed to get transfer command buffer");
    }

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;

    vkCmdCopyBuffer(
        commandBuffer,
        vulkanSource->getBuffer(),
        m_buffer,
        1,
        &copyRegion
    );

    auto submitResult = m_frameSync->submitTransferCommandBuffer();
    if (!submitResult) {
        return std::unexpected(
            "Failed to submit transfer command buffer: " + submitResult.error()
        );
    }

    auto waitResult = m_frameSync->waitForTransferComplete();
    if (!waitResult) {
        return std::unexpected(
            "Failed to wait for transfer completion: " + waitResult.error()
        );
    }

    Logger::debug(
        "Buffer copy completed: {} bytes from offset {} to offset {}",
        size,
        srcOffset,
        dstOffset
    );

    return {};
}

VulkanBuffer::VulkanBuffer(std::unique_ptr<Impl> impl)
    : m_impl(std::move(impl))
{}

VulkanBuffer::~VulkanBuffer() = default;

VulkanBuffer::VulkanBuffer(VulkanBuffer &&other) noexcept
    : m_impl(std::move(other.m_impl))
{}

auto VulkanBuffer::operator=(VulkanBuffer &&other) noexcept -> VulkanBuffer &
{
    if (this != &other) {
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

auto VulkanBuffer::create(
    VulkanInstance *instance,
    VulkanDevice *device,
    VulkanAllocator *allocator,
    VulkanFrameSync *frameSync,
    const graphics::BufferCreateInfo &info
) -> std::expected<std::unique_ptr<VulkanBuffer>, std::string>
{
    if (info.size == 0) {
        return std::unexpected("Buffer size must be greater than 0");
    }

    size_t requiredAlignment = utils::getRequiredAlignment(info.usage);

    if (info.size % requiredAlignment != 0) {
        Logger::warning(
            "Buffer size {} is not aligned to {} bytes for usage {}. "
            "This may cause performance issues or validation errors.",
            info.size,
            requiredAlignment,
            static_cast<int>(info.usage)
        );
    }

    Logger::debug(
        "Creating buffer: size={}, usage={}, memory={}, alignment={}",
        info.size,
        static_cast<int>(info.usage),
        static_cast<int>(info.memory),
        requiredAlignment
    );

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = info.size;
    bufferCreateInfo.usage = utils::toVulkanUsage(info.usage);

    auto memoryUsage = utils::toVmaMemoryUsage(info.memory);

    auto *transferQueue = device->getTransferQueue();
    if (transferQueue == nullptr) {
        return std::unexpected("Transfer queue is not set");
    }

    auto result = allocator->createBuffer(bufferCreateInfo, memoryUsage);
    if (!result) {
        return std::unexpected("Failed to create buffer: " + result.error());
    }

    auto [buffer, allocation] = result.value();

    if (!info.debugName.empty() && instance->hasValidation()) {
        bool success = utils::setDebugObjectName(
            device->getHandle(),
            VK_OBJECT_TYPE_BUFFER,
            std::bit_cast<u64>(buffer),
            info.debugName
        );

        if (!success) {
            return std::unexpected(
                "Failed to set debug name for buffer: " + info.debugName
            );
        }
    }

    auto impl = std::make_unique<Impl>(
        allocator,
        frameSync,
        transferQueue,
        buffer,
        allocation,
        info.usage,
        info.memory,
        info.size
    );

    if (!info.data.empty()) {
        auto updateResult = impl->update(info.data);
        if (!updateResult) {
            return std::unexpected(
                "Failed to initialize buffer with data: " + updateResult.error()
            );
        }
    }

    auto bufferPtr =
        std::unique_ptr<VulkanBuffer>(new VulkanBuffer(std::move(impl)));

    return bufferPtr;
}

auto VulkanBuffer::getBuffer() const -> VkBuffer
{
    return m_impl->getBuffer();
}

auto VulkanBuffer::getAllocation() const -> VmaAllocation
{
    return m_impl->getAllocation();
}

void VulkanBuffer::bind()
{
    m_impl->bind();
}

auto VulkanBuffer::map() -> std::expected<std::span<std::byte>, std::string>
{
    return m_impl->map();
}

void VulkanBuffer::unmap()
{
    m_impl->unmap();
}

auto VulkanBuffer::update(std::span<const std::byte> data, size_t offset)
    -> std::expected<void, std::string>
{
    return m_impl->update(data, offset);
}

auto VulkanBuffer::getSize() const -> size_t
{
    return m_impl->getSize();
}

auto VulkanBuffer::getUsage() const -> graphics::BufferUsage
{
    return m_impl->getUsage();
}

auto VulkanBuffer::getMemory() const -> graphics::BufferMemory
{
    return m_impl->getMemory();
}

auto VulkanBuffer::copyFrom(
    const graphics::Buffer &source,
    size_t srcOffset,
    size_t dstOffset,
    size_t size
) -> std::expected<void, std::string>
{
    return m_impl->copyFrom(source, srcOffset, dstOffset, size);
}

auto VulkanBuffer::getOffset() const -> size_t
{
    return m_impl->getOffset();
}

void VulkanBuffer::setOffset(size_t offset)
{
    m_impl->setOffset(offset);
}

} // namespace vostok::graphics::vulkan