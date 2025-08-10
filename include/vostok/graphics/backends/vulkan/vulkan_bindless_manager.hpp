#pragma once

#include "vostok/core/type.hpp"
#include "vostok/graphics/buffer.hpp"
#include "vostok/graphics/buffers/bindable_resource.hpp"
#include "vostok/graphics/buffers/ubo.hpp"

#include <expected>
#include <memory>
#include <unordered_map>
#include <vector>

namespace vostok::graphics::vulkan
{

class VulkanDevice;

struct VulkanBindlessManagerCreateInfo
{
    VulkanDevice *device;
    u32 maxUBOs;
    std::string debugName;
};

class VulkanBindlessManager
{
public:
    using CreateInfo = VulkanBindlessManagerCreateInfo;

    explicit VulkanBindlessManager(const CreateInfo &createInfo);
    ~VulkanBindlessManager();

    VulkanBindlessManager(const VulkanBindlessManager &) = delete;
    auto operator=(const VulkanBindlessManager &)
        -> VulkanBindlessManager & = delete;
    VulkanBindlessManager(VulkanBindlessManager &&) noexcept = default;
    auto operator=(VulkanBindlessManager &&) noexcept
        -> VulkanBindlessManager & = default;

    template <BindableType T>
    [[nodiscard]] auto createUBO(const T &initialData = T{})
        -> std::expected<std::unique_ptr<UBO<T>>, std::string>
    {
        auto ubo = std::make_unique<UBO<T>>(initialData);

        auto indexResult = registerUBO(&initialData, sizeof(T));
        if (!indexResult) {
            return std::unexpected(indexResult.error());
        }

        ubo->setBindlessIndex(indexResult.value());
        return ubo;
    }

    [[nodiscard]] auto registerUBO(const void *data, size_t size)
        -> std::expected<u32, std::string>;

    auto update() -> std::expected<void, std::string>;

private:
    VulkanDevice *m_device;

    u32 m_maxUBOs;

    struct BufferInfo
    {
        std::unique_ptr<Buffer> buffer;
        bool isDirty = true;

        BufferInfo(std::unique_ptr<Buffer> b)
            : buffer(std::move(b))
        {}
    };

    std::vector<BufferInfo> m_ubos;

    std::unordered_map<const void *, u32> m_resourceToIndex;
    bool m_isDirty = true;
};

} // namespace vostok::graphics::vulkan
