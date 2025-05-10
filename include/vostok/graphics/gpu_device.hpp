#pragma once

#include "vostok/core/type.hpp"
#include "vostok/core/version.hpp"

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

class GPUDevice
{
public:
    struct CreateInfo
    {
        std::string appName = "Vostok App";
        core::Version appVersion = core::Version{.major = 0, .minor = 1, .patch = 0};
        std::string engineName = "Vostok Engine";
        core::Version engineVersion = core::Version{.major = 0, .minor = 1, .patch = 0};
        void *windowHandle = nullptr;
        u32 width = 800;
        u32 height = 600;
        bool enableValidationLayers = true;
        bool enableVSync = true;
    };

    virtual ~GPUDevice() = default;

    GPUDevice(const GPUDevice &) = delete;
    GPUDevice &operator=(const GPUDevice &) = delete;
    GPUDevice(GPUDevice &&) = delete;
    GPUDevice &operator=(GPUDevice &&) = delete;

    static std::expected<std::unique_ptr<GPUDevice>, std::string>
    create(const CreateInfo &createInfo, RenderBackend backend = RenderBackend::VULKAN);

    virtual void waitIdle() = 0;

    virtual std::expected<u32, std::string> beginFrame() = 0;
    virtual std::expected<void, std::string> endFrame() = 0;

    virtual std::expected<void, std::string> resize(FramebufferSize size) = 0;
};

} // namespace vostok::graphics