#pragma once

#include "vostok/core/type.hpp"

#include <expected>
#include <memory>
#include <string>
#include <string_view>

namespace vostok
{

struct ProfilerConfig
{
    bool enableProfiling = true;
    bool enableGPUProfiling = true;
    bool enableMemoryProfiling = true;

    std::string serverAddress = "127.0.0.1";
    u16 serverPort = 8086;

    bool autoConnect = true;
    bool captureCallstack = true;

    std::string applicationName = "Vostok";
    std::string buildInfo;

    bool enableVulkanProfiling = true;
    bool profileTextureOperations = true;
    bool profileGPUMemory = true;
};

class Profiler
{
public:
    static auto init(const ProfilerConfig &config = ProfilerConfig{})
        -> std::expected<void, std::string>;
    static void shutdown();

    static auto isInitialized() -> bool;
    static auto getConfig() -> const ProfilerConfig &;

    static void beginFrame();
    static void endFrame();

    static void zone(std::string_view name);
    static void zoneValue(std::string_view name, u64 value);
    static void zoneText(std::string_view name, std::string_view text);

    static void beginGPUTask(std::string_view name);
    static void endGPUTask();

    static void
    beginTextureOperation(std::string_view operation, std::string_view path);
    static void endTextureOperation();

    static void trackAllocation(std::string_view name, size_t size);
    static void trackDeallocation(std::string_view name, size_t size);

    static void setCounter(std::string_view name, f64 value);
    static void plot(std::string_view name, f64 value);

private:
    Profiler() = default;

    class Impl;
    static std::unique_ptr<Impl> g_impl;
    static ProfilerConfig g_config;
    static bool g_initialized;
};

class ScopedZone
{
public:
    explicit ScopedZone(std::string_view name);
    ScopedZone(std::string_view name, u64 value);
    ScopedZone(std::string_view name, std::string_view text);

    ~ScopedZone();

    ScopedZone(const ScopedZone &) = delete;
    auto operator=(const ScopedZone &) -> ScopedZone & = delete;

    ScopedZone(ScopedZone &&other) noexcept;
    auto operator=(ScopedZone &&other) noexcept -> ScopedZone &;

private:
    std::string m_name;
    bool m_active = true;
};

class ScopedFrame
{
public:
    explicit ScopedFrame(std::string_view name = "Frame");
    ~ScopedFrame();

    ScopedFrame(const ScopedFrame &) = delete;
    auto operator=(const ScopedFrame &) -> ScopedFrame & = delete;

    ScopedFrame(ScopedFrame &&other) noexcept;
    auto operator=(ScopedFrame &&other) noexcept -> ScopedFrame &;

private:
    std::string m_name;
    bool m_active = true;
};

class ScopedGPUTask
{
public:
    explicit ScopedGPUTask(std::string_view name);
    ~ScopedGPUTask();

    ScopedGPUTask(const ScopedGPUTask &) = delete;
    auto operator=(const ScopedGPUTask &) -> ScopedGPUTask & = delete;

    ScopedGPUTask(ScopedGPUTask &&other) noexcept;
    auto operator=(ScopedGPUTask &&other) noexcept -> ScopedGPUTask &;

private:
    std::string m_name;
    bool m_active = true;
};

class ScopedTextureOperation
{
public:
    ScopedTextureOperation(
        std::string_view operation,
        std::string_view texturePath
    );
    ~ScopedTextureOperation();

    ScopedTextureOperation(const ScopedTextureOperation &) = delete;
    auto operator=(const ScopedTextureOperation &)
        -> ScopedTextureOperation & = delete;

    ScopedTextureOperation(ScopedTextureOperation &&other) noexcept;
    auto operator=(ScopedTextureOperation &&other) noexcept
        -> ScopedTextureOperation &;

private:
    std::string m_operation;
    std::string m_texturePath;
    bool m_active = true;
};

} // namespace vostok