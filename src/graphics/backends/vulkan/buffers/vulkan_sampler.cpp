#include "vostok/graphics/backends/vulkan/buffers/vulkan_sampler.hpp"

#include "vostok/core/logger/logger.hpp"
#include "vostok/graphics/backends/vulkan/core/vulkan_device.hpp"
#include "vostok/graphics/backends/vulkan/utils/vk_utils.hpp"

#include <format>
#include <volk.h>

namespace vostok::graphics::vulkan
{

auto VulkanSampler::create(VulkanDevice *device, const CreateInfo &createInfo)
    -> std::expected<std::unique_ptr<VulkanSampler>, std::string>
{
    if (device == nullptr) {
        return std::unexpected{ "Device cannot be null" };
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerInfo.magFilter = utils::toVulkanFilter(createInfo.magFilter);
    samplerInfo.minFilter = utils::toVulkanFilter(createInfo.minFilter);

    if (createInfo.enableMipmaps && createInfo.maxMipLevels > 1) {
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.maxLod = static_cast<f32>(createInfo.maxMipLevels);
        samplerInfo.minLod = static_cast<f32>(createInfo.minMipLevel);
        Logger::debug(
            "Creating mipmap sampler with {} levels",
            createInfo.maxMipLevels
        );
    } else {
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.maxLod = 0.0F;
        samplerInfo.minLod = 0.0F;
        Logger::debug("Creating non-mipmap sampler");
    }

    samplerInfo.addressModeU =
        utils::toVulkanAddressMode(createInfo.addressModeU);
    samplerInfo.addressModeV =
        utils::toVulkanAddressMode(createInfo.addressModeV);
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0F;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    VkSampler sampler = VK_NULL_HANDLE;
    VkResult result =
        vkCreateSampler(device->getHandle(), &samplerInfo, nullptr, &sampler);

    if (result != VK_SUCCESS) {
        return std::unexpected{ std::format(
            "Failed to create sampler: {}",
            utils::vkResultToString(result)
        ) };
    }

    Logger::debug("Created Vulkan sampler: {}", createInfo.debugName);
    return std::unique_ptr<VulkanSampler>(new VulkanSampler(device, sampler));
}

VulkanSampler::VulkanSampler(VulkanDevice *device, VkSampler sampler)
    : m_device(device),
      m_sampler(sampler)
{}

VulkanSampler::~VulkanSampler()
{
    if (m_sampler != VK_NULL_HANDLE && m_device != nullptr) {
        vkDestroySampler(m_device->getHandle(), m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
}

VulkanSampler::VulkanSampler(VulkanSampler &&other) noexcept
    : m_device(other.m_device),
      m_sampler(other.m_sampler)
{
    other.m_sampler = VK_NULL_HANDLE;
}

auto VulkanSampler::operator=(VulkanSampler &&other) noexcept -> VulkanSampler &
{
    if (this != &other) {
        if (m_sampler != VK_NULL_HANDLE && m_device != nullptr) {
            vkDestroySampler(m_device->getHandle(), m_sampler, nullptr);
        }

        m_device = other.m_device;
        m_sampler = other.m_sampler;
        other.m_sampler = VK_NULL_HANDLE;
    }
    return *this;
}

} // namespace vostok::graphics::vulkan