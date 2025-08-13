#pragma once

#include "vostok/core/logger/logger.hpp"
#include "vostok/core/type.hpp"
#include "vostok/core/version.hpp"
#include "vostok/graphics/buffer.hpp"
#include "vostok/graphics/buffers/ubo.hpp"
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

    virtual auto createPipeline(const PipelineCreateInfo &createInfo)
        -> std::expected<std::unique_ptr<PipelineHandle>, std::string> = 0;

    virtual auto createBuffer(const BufferCreateInfo &createInfo)
        -> std::expected<std::unique_ptr<Buffer>, std::string> = 0;

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

using GPU = std::unique_ptr<GPUHandle>;

} // namespace vostok::graphics