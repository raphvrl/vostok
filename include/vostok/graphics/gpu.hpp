#pragma once

#include "vostok/core/logger/logger.hpp"
#include "vostok/core/type.hpp"
#include "vostok/core/version.hpp"
#include "vostok/graphics/buffers/buffer.hpp"
#include "vostok/graphics/buffers/ibo.hpp"
#include "vostok/graphics/buffers/image.hpp"
#include "vostok/graphics/buffers/ubo.hpp"
#include "vostok/graphics/buffers/vbo.hpp"
#include "vostok/graphics/pipeline.hpp"

#include <expected>
#include <memory>
#include <string>

namespace vostok::graphics
{

enum RenderBackend : u8
{
    VULKAN,
};

struct FramebufferSize
{
    u32 width = 0;
    u32 height = 0;
};

class GPUHandle
{
public:
    struct CreateInfo
    {
        std::string appName = "Vostok App";
        core::Version appVersion =
            core::Version{ .major = 0, .minor = 1, .patch = 0 };
        std::string engineName = "Vostok Engine";
        core::Version engineVersion =
            core::Version{ .major = 0, .minor = 1, .patch = 0 };
        void *windowHandle = nullptr;
        u32 width = 800;
        u32 height = 600;
        bool enableValidationLayers = true;
        bool enableVSync = true;
    };

    GPUHandle() = default;
    virtual ~GPUHandle() = default;

    GPUHandle(const GPUHandle &) = delete;
    auto operator=(const GPUHandle &) -> GPUHandle & = delete;
    GPUHandle(GPUHandle &&) = delete;
    auto operator=(GPUHandle &&) -> GPUHandle & = delete;

    static auto create(
        const CreateInfo &createInfo,
        RenderBackend backend = RenderBackend::VULKAN
    ) -> std::expected<std::unique_ptr<GPUHandle>, std::string>;

    virtual void waitIdle() = 0;

    virtual auto beginFrame() -> std::expected<u32, std::string> = 0;
    virtual auto endFrame() -> std::expected<void, std::string> = 0;

    virtual auto resize(const FramebufferSize &size)
        -> std::expected<void, std::string> = 0;

    virtual void draw(
        u32 vertexCount,
        u32 instanceCount = 1,
        u32 firstVertex = 0,
        u32 firstInstance = 0
    ) = 0;

    virtual void drawIndexed(
        u32 indexCount,
        u32 instanceCount = 1,
        u32 firstIndex = 0,
        u32 vertexOffset = 0,
        u32 firstInstance = 0
    ) = 0;

    virtual auto createPipeline(const Pipeline::CreateInfo &createInfo)
        -> std::expected<Pipeline, std::string> = 0;

    virtual auto createBuffer(const BufferCreateInfo &createInfo)
        -> std::expected<std::unique_ptr<Buffer>, std::string> = 0;

    virtual auto createImage(const ImageCreateInfo &createInfo)
        -> std::expected<std::unique_ptr<Image>, std::string> = 0;

    auto createColorImage(
        u32 width,
        u32 height,
        ImageFormat format = ImageFormat::R8G8B8A8_UNORM,
        SampleCount samples = SampleCount::COUNT_1
    ) -> std::expected<std::unique_ptr<Image>, std::string>
    {
        ImageCreateInfo imageInfo;
        imageInfo.width = width;
        imageInfo.height = height;
        imageInfo.format = format;
        imageInfo.usage =
            ImageUsage::COLOR_ATTACHMENT | ImageUsage::TRANSFER_DST;
        imageInfo.samples = samples;
        imageInfo.debugName = "ColorImage_" + std::to_string(width) + "x" +
                              std::to_string(height);

        return createImage(imageInfo);
    }

    auto createDepthImage(
        u32 width,
        u32 height,
        ImageFormat format = ImageFormat::D32_SFLOAT,
        SampleCount samples = SampleCount::COUNT_1
    ) -> std::expected<std::unique_ptr<Image>, std::string>
    {
        ImageCreateInfo imageInfo;
        imageInfo.width = width;
        imageInfo.height = height;
        imageInfo.format = format;
        imageInfo.usage =
            ImageUsage::DEPTH_STENCIL_ATTACHMENT | ImageUsage::TRANSFER_DST;
        imageInfo.samples = samples;
        imageInfo.debugName = "DepthImage_" + std::to_string(width) + "x" +
                              std::to_string(height);

        return createImage(imageInfo);
    }

    auto createTextureImage(
        u32 width,
        u32 height,
        ImageFormat format = ImageFormat::R8G8B8A8_UNORM,
        u32 mipLevels = 1
    ) -> std::expected<std::unique_ptr<Image>, std::string>
    {
        ImageCreateInfo imageInfo;
        imageInfo.width = width;
        imageInfo.height = height;
        imageInfo.format = format;
        imageInfo.usage = ImageUsage::SAMPLED | ImageUsage::TRANSFER_DST;
        imageInfo.mipLevels = mipLevels;
        imageInfo.debugName = "TextureImage_" + std::to_string(width) + "x" +
                              std::to_string(height);

        return createImage(imageInfo);
    }

    template <typename T, typename... Formats>
        requires std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>
    auto createVBO(const std::span<const T> &data, Formats... formats)
        -> std::expected<VBO<T>, std::string>
    {
        const size_t SIZE = data.size() * sizeof(T);

        BufferCreateInfo bufferInfo;
        bufferInfo.usage = BufferUsage::VERTEX | BufferUsage::TRANSFER_DST;
        bufferInfo.memory = BufferMemory::GPU_ONLY;
        bufferInfo.size = SIZE;
        bufferInfo.data = std::span<const std::byte>(
            std::bit_cast<const std::byte *>(data.data()),
            SIZE
        );
        bufferInfo.debugName = "VBO_" + std::string(typeid(T).name());

        auto bufferResult = createBuffer(bufferInfo);
        if (!bufferResult) {
            return std::unexpected(
                "Failed to create buffer: " + bufferResult.error()
            );
        }

        auto layout = createVertexLayout({ formats... });

        auto vboImpl = std::make_unique<VBOImpl<T>>(
            std::move(bufferResult.value()),
            data,
            std::move(layout)
        );

        return VBO<T>(std::move(vboImpl));
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>
    auto createIBO(const std::span<const T> &data)
        -> std::expected<IBO<T>, std::string>
    {
        const size_t SIZE = data.size() * sizeof(T);

        BufferCreateInfo bufferInfo;
        bufferInfo.usage = BufferUsage::INDEX | BufferUsage::TRANSFER_DST;
        bufferInfo.memory = BufferMemory::GPU_ONLY;
        bufferInfo.size = SIZE;
        bufferInfo.data = std::span<const std::byte>(
            std::bit_cast<const std::byte *>(data.data()),
            SIZE
        );
        bufferInfo.debugName = "IBO_" + std::string(typeid(T).name());

        auto bufferResult = createBuffer(bufferInfo);
        if (!bufferResult) {
            return std::unexpected(
                "Failed to create buffer: " + bufferResult.error()
            );
        }

        auto iboImpl =
            std::make_unique<IBOImpl<T>>(std::move(bufferResult.value()), data);

        return IBO<T>(std::move(iboImpl));
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>
    auto createUBO(const T &initialData = T{})
        -> std::expected<UBO<T>, std::string>
    {
        auto ubo = std::make_unique<UBOImpl<T>>(initialData);

        auto result = registerUBO(ubo.get(), sizeof(T));
        if (!result) {
            return std::unexpected(result.error());
        }

        const u32 INDEX = result.value();
        ubo->setBindlessIndex(INDEX);

        ubo->setDirtyCallback([this, INDEX](const BindableResource *resource) {
            if (resource->getBindlessIndex() != INDEX) {
                Logger::error(
                    "Null resource in dirty callback for index {}",
                    INDEX
                );
                return;
            }

            notifyDirtyResource(INDEX);
        });

        return UBO<T>(std::move(ubo));
    }

private:
    virtual auto registerUBO(BindableResource *ubo, size_t size)
        -> std::expected<u32, std::string> = 0;

    virtual void notifyDirtyResource(u32 bindlessIndex) = 0;
};

struct GPU : public std::unique_ptr<GPUHandle>
{
    using Base = std::unique_ptr<GPUHandle>;
    using Base::Base;

    GPU() = default;
    ~GPU() = default;

    GPU(GPU &&) = default;
    auto operator=(GPU &&) -> GPU & = default;
    GPU(const GPU &) = delete;
    auto operator=(const GPU &) -> GPU & = delete;

    explicit GPU(std::unique_ptr<GPUHandle> &&ptr)
        : Base(std::move(ptr))
    {}

    using CreateInfo = GPUHandle::CreateInfo;

    static auto create(
        const CreateInfo &createInfo,
        RenderBackend backend = RenderBackend::VULKAN
    ) -> std::expected<GPU, std::string>
    {
        auto result = GPUHandle::create(createInfo, backend);
        if (!result) {
            return std::unexpected(result.error());
        }
        return GPU(std::move(result.value()));
    }
};

} // namespace vostok::graphics