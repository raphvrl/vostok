#include "vostok/graphics/backends/vulkan/vulkan_bindless_manager.hpp"

#include "core/logger/logger.hpp"
#include "graphics/backends/vulkan/buffers/vulkan_buffer.hpp"
#include "graphics/backends/vulkan/core/vulkan_device.hpp"
#include "graphics/backends/vulkan/core/vulkan_frame_sync.hpp"
#include "graphics/backends/vulkan/utils/vk_utils.hpp"

#include <format>
#include <utility>
#include <volk.h>

namespace vostok::graphics::vulkan
{

auto VulkanBindlessManager::create(const CreateInfo &createInfo)
    -> std::expected<std::unique_ptr<VulkanBindlessManager>, std::string>
{
    if (createInfo.instance == nullptr) {
        return std::unexpected{
            "VulkanBindlessManager: instance cannot be null"
        };
    }

    if (createInfo.device == nullptr) {
        return std::unexpected{
            "VulkanBindlessManager: device cannot be null"
        };
    }

    if (createInfo.frameSync == nullptr) {
        return std::unexpected{
            "VulkanBindlessManager: frameSync cannot be null"
        };
    }

    if (createInfo.allocator == nullptr) {
        return std::unexpected{
            "VulkanBindlessManager: allocator cannot be null"
        };
    }

    if (createInfo.maxUBOs == 0) {
        return std::unexpected{ "VulkanBindlessManager: maxUBOs cannot be 0" };
    }

    if (!createInfo.device->supportsBindlessResources()) {
        return std::unexpected{
            "VulkanBindlessManager: device does not support bindless resources"
        };
    }

    const auto MAX_DESCRIPTION_SETS = createInfo.device->getMaxDescriptorSets();
    if (createInfo.maxUBOs > MAX_DESCRIPTION_SETS) {
        return std::unexpected{ std::format(
            "VulkanBindlessManager: maxUBOs ({}) exceeds device limit ({})",
            createInfo.maxUBOs,
            MAX_DESCRIPTION_SETS
        ) };
    }

    try {
        auto manager = std::unique_ptr<VulkanBindlessManager>(
            new VulkanBindlessManager(createInfo)
        );

        auto initResult = manager->initBindlessResources();
        if (!initResult) {
            return std::unexpected{ std::format(
                "Failed to initialize bindless resources: {}",
                initResult.error()
            ) };
        }

        return manager;
    } catch (const std::exception &e) {
        return std::unexpected{
            std::format("VulkanBindlessManager creation failed: {}", e.what())
        };
    }
}

VulkanBindlessManager::VulkanBindlessManager(const CreateInfo &createInfo)
    : m_instance{ createInfo.instance },
      m_device{ createInfo.device },
      m_frameSync{ createInfo.frameSync },
      m_allocator{ createInfo.allocator },
      m_maxUBOs{ createInfo.maxUBOs }
{
    m_uboToIndex.reserve(m_maxUBOs);
    m_dirtyStack.reserve(m_maxUBOs);
    m_gpuBuffers.reserve(m_maxUBOs);
    m_bufferSizes.reserve(m_maxUBOs);
}

VulkanBindlessManager::~VulkanBindlessManager()
{
    cleanupBindlessResources();

    if (m_uboToIndex.empty()) {
        Logger::debug(
            "VulkanBindlessManager destroyed with no registered UBOs"
        );
    }
}

VulkanBindlessManager::VulkanBindlessManager(
    VulkanBindlessManager &&other
) noexcept
    : m_instance{ std::exchange(other.m_instance, nullptr) },
      m_device{ std::exchange(other.m_device, nullptr) },
      m_frameSync{ std::exchange(other.m_frameSync, nullptr) },
      m_allocator{ std::exchange(other.m_allocator, nullptr) },
      m_maxUBOs{ std::exchange(other.m_maxUBOs, 0) },
      m_uboToIndex{ std::move(other.m_uboToIndex) },
      m_dirtyStack{ std::move(other.m_dirtyStack) },
      m_descriptorSetLayout{
          std::exchange(other.m_descriptorSetLayout, VK_NULL_HANDLE)
      },
      m_descriptorPool{ std::exchange(other.m_descriptorPool, VK_NULL_HANDLE) },
      m_descriptorSet{ std::exchange(other.m_descriptorSet, VK_NULL_HANDLE) },
      m_gpuBuffers{ std::move(other.m_gpuBuffers) },
      m_bufferSizes{ std::move(other.m_bufferSizes) }
{}

auto VulkanBindlessManager::operator=(VulkanBindlessManager &&other) noexcept
    -> VulkanBindlessManager &
{
    if (this != &other) {
        cleanupBindlessResources();

        m_instance = std::exchange(other.m_instance, nullptr);
        m_device = std::exchange(other.m_device, nullptr);
        m_frameSync = std::exchange(other.m_frameSync, nullptr);
        m_allocator = std::exchange(other.m_allocator, nullptr);
        m_maxUBOs = std::exchange(other.m_maxUBOs, 0);
        m_uboToIndex = std::move(other.m_uboToIndex);
        m_dirtyStack = std::move(other.m_dirtyStack);
        m_gpuBuffers = std::move(other.m_gpuBuffers);
        m_bufferSizes = std::move(other.m_bufferSizes);
        m_descriptorSetLayout =
            std::exchange(other.m_descriptorSetLayout, VK_NULL_HANDLE);
        m_descriptorPool =
            std::exchange(other.m_descriptorPool, VK_NULL_HANDLE);
        m_descriptorSet = std::exchange(other.m_descriptorSet, VK_NULL_HANDLE);
    }
    return *this;
}

auto VulkanBindlessManager::initBindlessResources()
    -> std::expected<void, std::string>
{
    auto *device = m_device->getHandle();

    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_ALL;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    layoutInfo.flags =
        VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    VkResult result = vkCreateDescriptorSetLayout(
        device,
        &layoutInfo,
        nullptr,
        &m_descriptorSetLayout
    );

    if (result != VK_SUCCESS) {
        return std::unexpected{ std::format(
            "Failed to create descriptor set layout: {}",
            utils::vkResultToString(result)
        ) };
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = m_maxUBOs;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT |
                     VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    result =
        vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool);

    if (result != VK_SUCCESS) {
        return std::unexpected{ std::format(
            "Failed to create descriptor pool: {}",
            utils::vkResultToString(result)
        ) };
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayout;

    result = vkAllocateDescriptorSets(device, &allocInfo, &m_descriptorSet);
    if (result != VK_SUCCESS) {
        return std::unexpected{ std::format(
            "Failed to allocate descriptor set: {}",
            utils::vkResultToString(result)
        ) };
    }

    return {};
}

void VulkanBindlessManager::cleanupBindlessResources()
{
    auto *device = m_device->getHandle();

    if (m_descriptorSet != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(device, m_descriptorPool, 1, &m_descriptorSet);
        m_descriptorSet = VK_NULL_HANDLE;
    }

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    m_gpuBuffers.clear();
    m_bufferSizes.clear();
}

auto VulkanBindlessManager::registerUBO(
    graphics::BindableResource *ubo,
    size_t size
) -> std::expected<u32, std::string>
{
    if (ubo == nullptr) {
        return std::unexpected("UBO null");
    }

    if (m_uboToIndex.size() >= m_maxUBOs) {
        return std::unexpected("Nombre maximum d'UBO atteint");
    }

    const u32 INDEX = static_cast<u32>(m_uboToIndex.size());
    m_uboToIndex[ubo] = INDEX;
    m_indexToUBO[INDEX] = ubo;

    auto bufferResult = createGPUBuffer(size, ubo->getRawData());
    if (!bufferResult) {
        m_uboToIndex.erase(ubo);
        return std::unexpected(
            std::format("Échec création buffer GPU: {}", bufferResult.error())
        );
    }

    m_gpuBuffers.push_back(std::move(bufferResult.value()));
    m_bufferSizes.push_back(size);

    auto descUpdateResult = updateDescriptorSet(
        INDEX,
        dynamic_cast<VulkanBuffer *>(m_gpuBuffers[INDEX].get())->getBuffer(),
        size
    );

    if (!descUpdateResult) {
        Logger::warning(
            "Failed to update descriptor set for UBO {}: {}",
            INDEX,
            descUpdateResult.error()
        );
    }

    Logger::debug(
        "UBO enregistré à l'index {} (total: {})",
        INDEX,
        m_uboToIndex.size()
    );

    return INDEX;
}

auto VulkanBindlessManager::unregisterUBO(const BindableResource *ubo)
    -> std::expected<void, std::string>
{
    if (ubo == nullptr) {
        return std::unexpected{ "Cannot unregister null UBO" };
    }

    const auto IT = m_uboToIndex.find(ubo);
    if (IT == m_uboToIndex.end()) {
        return std::unexpected{ "UBO not registered" };
    }

    const u32 INDEX = IT->second;
    m_uboToIndex.erase(IT);
    m_indexToUBO.erase(INDEX);

    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    const auto DIRTY_IT = std::ranges::find(m_dirtyStack, ubo);
    if (DIRTY_IT != m_dirtyStack.end()) {
        m_dirtyStack.erase(DIRTY_IT);
    }

    if (INDEX < m_gpuBuffers.size()) {
        m_gpuBuffers.erase(m_gpuBuffers.begin() + INDEX);
        m_bufferSizes.erase(m_bufferSizes.begin() + INDEX);
    }

    Logger::debug(
        "Unregistered UBO at index {} (total: {})",
        INDEX,
        m_uboToIndex.size()
    );

    return {};
}

auto VulkanBindlessManager::update() -> std::expected<void, std::string>
{
    std::vector<const BindableResource *> dirtyResources;

    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    if (m_dirtyStack.empty()) {
        return {};
    }

    dirtyResources.swap(m_dirtyStack);

    Logger::debug("Updating {} dirty UBOs", dirtyResources.size());

    for (const auto *resource : dirtyResources) {
        const auto IT = m_uboToIndex.find(resource);
        if (IT == m_uboToIndex.end()) {
            Logger::warning("Dirty resource not found in UBO map, skipping");
            continue;
        }

        const u32 INDEX = IT->second;
        auto updateResult = updateUBO(INDEX, resource);
        if (!updateResult) {
            return std::unexpected{ std::format(
                "Failed to update UBO at index {}: {}",
                INDEX,
                updateResult.error()
            ) };
        }
    }

    return {};
}

void VulkanBindlessManager::notifyDirty(u32 bindlessIndex)
{
    const auto IT = m_indexToUBO.find(bindlessIndex);
    if (IT == m_indexToUBO.end()) {
        Logger::warning("Dirty index not found: {}", bindlessIndex);
        return;
    }

    const auto *resource = IT->second;
    std::lock_guard<std::mutex> lock(m_dirtyMutex);

    if (std::ranges::find(m_dirtyStack, resource) == m_dirtyStack.end()) {
        m_dirtyStack.push_back(resource);
    }
}

auto VulkanBindlessManager::updateUBO(
    u32 index,
    const BindableResource *resource
) -> std::expected<void, std::string>
{
    if (resource == nullptr) {
        return std::unexpected{ "Cannot update null resource" };
    }

    try {
        const auto *data = resource->getRawData();
        auto size = resource->getDataSize();

        if ((data == nullptr) || size == 0) {
            return std::unexpected{ "Invalid resource data" };
        }

        if (index >= m_gpuBuffers.size()) {
            return std::unexpected{
                std::format("Invalid UBO index: {}", index)
            };
        }

        auto updateResult = m_gpuBuffers[index]->update(
            std::span<const std::byte>(
                static_cast<const std::byte *>(data),
                size
            )
        );

        if (!updateResult) {
            return std::unexpected{ std::format(
                "Failed to update GPU buffer: {}",
                updateResult.error()
            ) };
        }

        if (m_bufferSizes[index] != size) {
            m_bufferSizes[index] = size;
            auto descUpdateResult = updateDescriptorSet(
                index,
                dynamic_cast<VulkanBuffer *>(m_gpuBuffers[index].get())
                    ->getBuffer(),
                size
            );

            if (!descUpdateResult) {
                Logger::warning(
                    "Failed to update descriptor set: {}",
                    descUpdateResult.error()
                );
            }
        }

        return {};
    } catch (const std::exception &e) {
        return std::unexpected{
            std::format("Failed to update UBO at index {}: {}", index, e.what())
        };
    }
}

auto VulkanBindlessManager::createGPUBuffer(size_t size, const void *data)
    -> std::expected<std::unique_ptr<Buffer>, std::string>
{
    if (size == 0) {
        return std::unexpected{ "Buffer size cannot be zero" };
    }

    if (data == nullptr) {
        return std::unexpected{ "Buffer data cannot be null" };
    }

    try {
        graphics::BufferCreateInfo bufferInfo{};
        bufferInfo.usage = graphics::BufferUsage::UNIFORM |
                           graphics::BufferUsage::TRANSFER_DST;
        bufferInfo.memory = graphics::BufferMemory::GPU_ONLY;
        bufferInfo.size = size;

        bufferInfo.data = std::span<const std::byte>(
            static_cast<const std::byte *>(data),
            size
        );

        auto buffer = VulkanBuffer::create(
            m_instance,
            m_device,
            m_allocator,
            m_frameSync,
            bufferInfo
        );

        return buffer;
    } catch (const std::exception &e) {
        return std::unexpected{
            std::format("Failed to create GPU buffer: {}", e.what())
        };
    }
}

auto VulkanBindlessManager::updateDescriptorSet(
    u32 index,
    VkBuffer buffer,
    size_t size
) -> std::expected<void, std::string>
{
    if (m_descriptorSet == VK_NULL_HANDLE) {
        return std::unexpected{ "Descriptor set not initialized" };
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = size;

    VkWriteDescriptorSet writeInfo{};
    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstSet = m_descriptorSet;
    writeInfo.dstBinding = 0;
    writeInfo.dstArrayElement = index;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeInfo.descriptorCount = 1;
    writeInfo.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(m_device->getHandle(), 1, &writeInfo, 0, nullptr);

    return {};
}

} // namespace vostok::graphics::vulkan