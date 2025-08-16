#include "vostok/graphics/backends/vulkan/vulkan_bindless_manager.hpp"

#include "core/logger/logger.hpp"
#include "graphics/backends/vulkan/buffers/vulkan_buffer.hpp"
#include "graphics/backends/vulkan/buffers/vulkan_image.hpp"
#include "graphics/backends/vulkan/buffers/vulkan_sampler.hpp"
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
      m_maxUBOs{ createInfo.maxUBOs },
      m_maxTextures{ createInfo.maxTextures }
{
    m_uboToIndex.reserve(m_maxUBOs);
    m_dirtyStack.reserve(m_maxUBOs);
    m_gpuBuffers.reserve(m_maxUBOs);
    m_bufferSizes.reserve(m_maxUBOs);
    m_textureToIndex.reserve(m_maxTextures);
    m_indexToTexture.reserve(m_maxTextures);
    m_gpuImages.reserve(m_maxTextures);
}

VulkanBindlessManager::~VulkanBindlessManager()
{
    cleanupBindlessResources();
}

VulkanBindlessManager::VulkanBindlessManager(
    VulkanBindlessManager &&other
) noexcept
    : m_instance{ std::exchange(other.m_instance, nullptr) },
      m_device{ std::exchange(other.m_device, nullptr) },
      m_frameSync{ std::exchange(other.m_frameSync, nullptr) },
      m_allocator{ std::exchange(other.m_allocator, nullptr) },
      m_maxUBOs{ std::exchange(other.m_maxUBOs, 0) },
      m_maxTextures{ std::exchange(other.m_maxTextures, 0) },
      m_uboToIndex{ std::move(other.m_uboToIndex) },
      m_indexToUBO{ std::move(other.m_indexToUBO) },
      m_textureToIndex{ std::move(other.m_textureToIndex) },
      m_indexToTexture{ std::move(other.m_indexToTexture) },
      m_dirtyStack{ std::move(other.m_dirtyStack) },
      m_descriptorSetLayout{
          std::exchange(other.m_descriptorSetLayout, VK_NULL_HANDLE)
      },
      m_descriptorPool{ std::exchange(other.m_descriptorPool, VK_NULL_HANDLE) },
      m_descriptorSet{ std::exchange(other.m_descriptorSet, VK_NULL_HANDLE) },
      m_gpuBuffers{ std::move(other.m_gpuBuffers) },
      m_bufferSizes{ std::move(other.m_bufferSizes) },
      m_gpuImages{ std::move(other.m_gpuImages) }
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

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = m_maxUBOs;
    uboBinding.stageFlags = VK_SHADER_STAGE_ALL;
    uboBinding.pImmutableSamplers = nullptr;
    bindings.push_back(uboBinding);

    VkDescriptorSetLayoutBinding ssboBinding{};
    ssboBinding.binding = 1;
    ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboBinding.descriptorCount = m_maxUBOs;
    ssboBinding.stageFlags = VK_SHADER_STAGE_ALL;
    ssboBinding.pImmutableSamplers = nullptr;
    bindings.push_back(ssboBinding);

    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 2;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = m_maxTextures;
    textureBinding.stageFlags = VK_SHADER_STAGE_ALL;
    textureBinding.pImmutableSamplers = nullptr;
    bindings.push_back(textureBinding);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<u32>(bindings.size());
    layoutInfo.pBindings = bindings.data();
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

    std::vector<VkDescriptorPoolSize> poolSizes;

    VkDescriptorPoolSize uboPoolSize{};
    uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboPoolSize.descriptorCount = m_maxUBOs;
    poolSizes.push_back(uboPoolSize);

    VkDescriptorPoolSize ssboPoolSize{};
    ssboPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboPoolSize.descriptorCount = m_maxUBOs;
    poolSizes.push_back(ssboPoolSize);

    VkDescriptorPoolSize texturePoolSize{};
    texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texturePoolSize.descriptorCount = m_maxTextures;
    poolSizes.push_back(texturePoolSize);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
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

    auto samplerResult = createGlobalSampler();
    if (!samplerResult) {
        return samplerResult;
    }

    return {};
}

void VulkanBindlessManager::cleanupBindlessResources()
{
    auto *device = m_device->getHandle();

    if (m_globalSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_globalSampler, nullptr);
        m_globalSampler = VK_NULL_HANDLE;
    }

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

auto VulkanBindlessManager::createGlobalSampler()
    -> std::expected<void, std::string>
{
    auto *device = m_device->getHandle();

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0F;
    samplerInfo.minLod = 0.0F;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    VkResult result =
        vkCreateSampler(device, &samplerInfo, nullptr, &m_globalSampler);
    if (result != VK_SUCCESS) {
        return std::unexpected{ std::format(
            "Failed to create global sampler: {}",
            utils::vkResultToString(result)
        ) };
    }

    Logger::debug("Global sampler created successfully");
    return {};
}

auto VulkanBindlessManager::unregisterResource(
    ResourceType type,
    const graphics::BindableResource *resource
) -> std::expected<void, std::string>
{
    switch (type) {
        case ResourceType::UBO:
            return unregisterUBO(resource);
        case ResourceType::TEXTURE:
            return unregisterTexture(resource);
        default:
            return std::unexpected{ "Invalid resource type" };
    }
}

auto VulkanBindlessManager::update() -> std::expected<void, std::string>
{
    std::vector<const graphics::BindableResource *> dirtyResources;

    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    if (m_dirtyStack.empty()) {
        return {};
    }

    dirtyResources.swap(m_dirtyStack);

    Logger::trace("Updating {} dirty resources", dirtyResources.size());

    for (const auto *resource : dirtyResources) {
        const auto UBO_IT = m_uboToIndex.find(resource);
        if (UBO_IT != m_uboToIndex.end()) {
            const u32 INDEX = UBO_IT->second;
            auto updateResult = updateUBO(INDEX, resource);
            if (!updateResult) {
                return std::unexpected{ std::format(
                    "Failed to update UBO at index {}: {}",
                    INDEX,
                    updateResult.error()
                ) };
            }
            continue;
        }

        Logger::warning("Dirty resource not found in any map, skipping");
    }

    return {};
}

void VulkanBindlessManager::notifyDirty(u32 bindlessIndex)
{
    const auto UBO_IT = m_indexToUBO.find(bindlessIndex);
    if (UBO_IT != m_indexToUBO.end()) {
        const auto *resource = UBO_IT->second;
        std::lock_guard<std::mutex> lock(m_dirtyMutex);

        if (std::ranges::find(m_dirtyStack, resource) == m_dirtyStack.end()) {
            m_dirtyStack.push_back(resource);
        }
        return;
    }

    Logger::warning("Dirty index not found: {}", bindlessIndex);
}

auto VulkanBindlessManager::registerUBO(
    graphics::BindableResource *ubo,
    size_t size
) -> std::expected<u32, std::string>
{
    if (ubo == nullptr) {
        return std::unexpected("UBO is null");
    }

    if (m_uboToIndex.size() >= m_maxUBOs) {
        return std::unexpected("Maximum number of UBOs reached");
    }

    const u32 INDEX = static_cast<u32>(m_uboToIndex.size());
    m_uboToIndex[ubo] = INDEX;
    m_indexToUBO[INDEX] = ubo;

    auto bufferResult = createGPUBuffer(size, ubo->getRawData());
    if (!bufferResult) {
        m_uboToIndex.erase(ubo);
        return std::unexpected(
            std::format("Failed to create GPU buffer: {}", bufferResult.error())
        );
    }

    m_gpuBuffers.push_back(std::move(bufferResult.value()));
    m_bufferSizes.push_back(size);

    auto descUpdateResult = updateUBODescriptorSet(
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
        "UBO registered at index {} (total: {})",
        INDEX,
        m_uboToIndex.size()
    );

    return INDEX;
}

auto VulkanBindlessManager::registerTexture(graphics::BindableResource *texture)
    -> std::expected<u32, std::string>
{
    if (texture == nullptr) {
        return std::unexpected("Texture is null");
    }

    if (m_textureToIndex.size() >= m_maxTextures) {
        return std::unexpected("Maximum number of textures reached");
    }

    const u32 INDEX = static_cast<u32>(m_textureToIndex.size());
    m_textureToIndex[texture] = INDEX;
    m_indexToTexture[INDEX] = texture;

    auto imageResult =
        createGPUImage(dynamic_cast<graphics::TextureImpl *>(texture));
    if (!imageResult) {
        m_textureToIndex.erase(texture);
        m_indexToTexture.erase(INDEX);
        return std::unexpected(
            std::format("Failed to create GPU texture: {}", imageResult.error())
        );
    }

    m_gpuImages.push_back(std::move(imageResult.value()));

    auto *textureImpl = dynamic_cast<graphics::TextureImpl *>(texture);
    auto samplerResult = createSamplerForTexture(textureImpl);
    if (!samplerResult) {
        Logger::warning(
            "Failed to create sampler for texture {}, using global sampler: {}",
            INDEX,
            samplerResult.error()
        );
        m_gpuSamplers.push_back(nullptr);
    } else {
        m_gpuSamplers.push_back(std::move(samplerResult.value()));
    }

    VkSampler sampler = m_gpuSamplers[INDEX] ? m_gpuSamplers[INDEX]->getHandle()
                                             : m_globalSampler;

    auto descUpdateResult = updateTextureDescriptorSet(
        INDEX,
        dynamic_cast<VulkanImage *>(m_gpuImages[INDEX].get())->getImageView(),
        sampler
    );

    if (!descUpdateResult) {
        Logger::warning(
            "Failed to update texture descriptor set for texture {}: {}",
            INDEX,
            descUpdateResult.error()
        );
    }

    Logger::debug(
        "Texture registered at index {} (total: {})",
        INDEX,
        m_textureToIndex.size()
    );

    return INDEX;
}

auto VulkanBindlessManager::unregisterUBO(const graphics::BindableResource *ubo)
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

auto VulkanBindlessManager::unregisterTexture(
    const graphics::BindableResource *texture
) -> std::expected<void, std::string>
{
    if (texture == nullptr) {
        return std::unexpected{ "Cannot unregister null texture" };
    }

    const auto IT = m_textureToIndex.find(texture);
    if (IT == m_textureToIndex.end()) {
        return std::unexpected{ "Texture not registered" };
    }

    const u32 INDEX = IT->second;
    m_textureToIndex.erase(IT);
    m_indexToTexture.erase(INDEX);

    if (INDEX < m_gpuImages.size()) {
        m_gpuImages.erase(m_gpuImages.begin() + INDEX);
    }

    if (INDEX < m_gpuSamplers.size()) {
        m_gpuSamplers.erase(m_gpuSamplers.begin() + INDEX);
    }

    Logger::debug(
        "Unregistered texture at index {} (total: {})",
        INDEX,
        m_textureToIndex.size()
    );

    return {};
}

auto VulkanBindlessManager::updateUBO(
    u32 index,
    const graphics::BindableResource *resource
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
            auto descUpdateResult = updateUBODescriptorSet(
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

auto VulkanBindlessManager::createGPUImage(graphics::TextureImpl *texture)
    -> std::expected<std::unique_ptr<VulkanImage>, std::string>
{
    if (texture == nullptr) {
        return std::unexpected{ "TextureImpl cannot be null" };
    }

    graphics::BufferCreateInfo stagingInfo{};
    stagingInfo.usage = graphics::BufferUsage::TRANSFER_SRC;
    stagingInfo.memory = graphics::BufferMemory::CPU_TO_GPU;
    stagingInfo.size = texture->getDataSize();
    stagingInfo.data = std::span<const std::byte>(
        static_cast<const std::byte *>(texture->getRawData()),
        texture->getDataSize()
    );

    auto stagingBufferResult = VulkanBuffer::create(
        m_instance,
        m_device,
        m_allocator,
        m_frameSync,
        stagingInfo
    );

    if (!stagingBufferResult) {
        return std::unexpected{ "Failed to create staging buffer" };
    }

    graphics::ImageCreateInfo imageInfo{};
    imageInfo.width = texture->getWidth();
    imageInfo.height = texture->getHeight();
    imageInfo.depth = 1;
    imageInfo.mipLevels = texture->getMipLevels();
    imageInfo.arrayLayers = 1;
    imageInfo.format = texture->getFormat();
    imageInfo.usage = texture->getUsage();
    imageInfo.samples = graphics::SampleCount::COUNT_1;

    auto imageResult = VulkanImage::createAndTransfer(
        m_device,
        m_allocator,
        m_frameSync,
        imageInfo,
        std::move(stagingBufferResult.value())
    );

    if (!imageResult) {
        return std::unexpected{ std::format(
            "Failed to create and transfer image: {}",
            imageResult.error()
        ) };
    }

    return imageResult;
}

auto VulkanBindlessManager::updateUBODescriptorSet(
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

auto VulkanBindlessManager::updateSSBODescriptorSet(
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
    writeInfo.dstBinding = 1;
    writeInfo.dstArrayElement = index;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeInfo.descriptorCount = 1;
    writeInfo.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(m_device->getHandle(), 1, &writeInfo, 0, nullptr);

    return {};
}

auto VulkanBindlessManager::updateTextureDescriptorSet(
    u32 index,
    VkImageView imageView,
    VkSampler sampler
) -> std::expected<void, std::string>
{
    if (m_descriptorSet == VK_NULL_HANDLE) {
        return std::unexpected{ "Descriptor set not initialized" };
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet writeInfo{};

    writeInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstSet = m_descriptorSet;
    writeInfo.dstBinding = 2;
    writeInfo.dstArrayElement = index;
    writeInfo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeInfo.descriptorCount = 1;
    writeInfo.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device->getHandle(), 1, &writeInfo, 0, nullptr);

    return {};
}

auto VulkanBindlessManager::createSamplerForTexture(
    graphics::TextureImpl *texture
) -> std::expected<std::unique_ptr<VulkanSampler>, std::string>
{
    VulkanSampler::CreateInfo samplerInfo{};

    samplerInfo.enableMipmaps = (texture->getMipLevels() > 1);
    samplerInfo.maxMipLevels = texture->getMipLevels();
    samplerInfo.minMipLevel = texture->getMinMipLevel();
    samplerInfo.magFilter = texture->getMagFilter();
    samplerInfo.minFilter = texture->getMinFilter();
    samplerInfo.addressModeU = graphics::AddressMode::REPEAT;
    samplerInfo.addressModeV = graphics::AddressMode::REPEAT;

    samplerInfo.debugName = std::format(
        "TextureSampler_{}x{}_{}mips",
        texture->getWidth(),
        texture->getHeight(),
        texture->getMipLevels()
    );

    Logger::debug(
        "Creating sampler for texture: {}x{} with {} mip levels",
        texture->getWidth(),
        texture->getHeight(),
        texture->getMipLevels()
    );

    return VulkanSampler::create(m_device, samplerInfo);
}

} // namespace vostok::graphics::vulkan