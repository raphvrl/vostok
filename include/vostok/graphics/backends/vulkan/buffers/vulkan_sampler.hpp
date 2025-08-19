#pragma once

#include "vostok/graphics/textures/texture.hpp"

#include <expected>
#include <memory>
#include <string>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanDevice;

struct SamplerCreateInfo
{
    graphics::Filter magFilter = graphics::Filter::LINEAR;
    graphics::Filter minFilter = graphics::Filter::LINEAR;
    graphics::AddressMode addressModeU = graphics::AddressMode::REPEAT;
    graphics::AddressMode addressModeV = graphics::AddressMode::REPEAT;
    bool enableMipmaps = true;
    u32 maxMipLevels = 1;
    u32 minMipLevel = 0;
    std::string debugName = "VulkanSampler";
};

class VulkanSampler
{
public:
    using CreateInfo = SamplerCreateInfo;

    static auto create(VulkanDevice *device, const CreateInfo &createInfo)
        -> std::expected<std::unique_ptr<VulkanSampler>, std::string>;

    ~VulkanSampler();

    VulkanSampler(const VulkanSampler &) = delete;
    auto operator=(const VulkanSampler &) -> VulkanSampler & = delete;
    VulkanSampler(VulkanSampler &&other) noexcept;
    auto operator=(VulkanSampler &&other) noexcept -> VulkanSampler &;

    [[nodiscard]] auto getHandle() const -> VkSampler { return m_sampler; }

private:
    VulkanSampler(VulkanDevice *device, VkSampler sampler);

    VulkanDevice *m_device = nullptr;
    VkSampler m_sampler = VK_NULL_HANDLE;
};

} // namespace vostok::graphics::vulkan