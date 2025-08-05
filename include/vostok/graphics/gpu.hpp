#pragma once

#include "vostok/core/type.hpp"
#include "vostok/core/version.hpp"
#include "vostok/graphics/buffer.hpp"
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

class GPU
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

    GPU() = default;
    virtual ~GPU() = default;

    GPU(const GPU &) = delete;
    auto operator=(const GPU &) -> GPU & = delete;
    GPU(GPU &&) = delete;
    auto operator=(GPU &&) -> GPU & = delete;

    static auto create(
        const CreateInfo &createInfo,
        RenderBackend backend = RenderBackend::VULKAN
    ) -> std::expected<std::unique_ptr<GPU>, std::string>;

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

    virtual auto createPipelineBuilder()
        -> std::expected<std::unique_ptr<Pipeline::Builder>, std::string> = 0;

    virtual auto createBuffer(const BufferCreateInfo &createInfo)
        -> std::expected<std::unique_ptr<Buffer>, std::string> = 0;
};

} // namespace vostok::graphics