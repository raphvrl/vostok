#include "core/profiler/profiler.hpp"

#include "core/logger/logger.hpp"

#include <chrono>
#include <expected>
#include <format>
#include <memory>
#include <string>
#include <tracy/Tracy.hpp>
#include <utility>

namespace vostok
{

class Profiler::Impl
{
public:
    explicit Impl(ProfilerConfig config);
    ~Impl() = default;

    Impl(const Impl &) = delete;
    Impl(Impl &&) noexcept = default;
    auto operator=(const Impl &) -> Impl & = delete;
    auto operator=(Impl &&) noexcept -> Impl & = default;

    void beginFrame();
    void endFrame();

    void zone(std::string_view name) const;
    void zoneValue(std::string_view name, u64 value) const;
    void zoneText(std::string_view name, std::string_view text) const;

    void beginGPUTask(std::string_view name) const;
    void endGPUTask() const;

    void beginTextureOperation(
        std::string_view operation,
        std::string_view path
    ) const;
    void endTextureOperation() const;

    void trackAllocation(std::string_view name, size_t size) const;
    void trackDeallocation(std::string_view name, size_t size) const;

    void setCounter(std::string_view name, f64 value) const;
    void plot(std::string_view name, f64 value) const;

private:
    static void initializeTracy();

    ProfilerConfig m_config;
    u64 m_frameCount = 0;
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_currentFrameStart;

    bool m_tracyEnabled = false;
};

std::unique_ptr<Profiler::Impl> Profiler::g_impl = nullptr;
ProfilerConfig Profiler::g_config = ProfilerConfig{};
bool Profiler::g_initialized = false;

Profiler::Impl::Impl(ProfilerConfig config)
    : m_config(std::move(config)),
      m_startTime(std::chrono::steady_clock::now()),
      m_tracyEnabled(m_config.enableProfiling)
{
    if (m_tracyEnabled) {
        initializeTracy();
    }

    Logger::info(
        "Profiler initialized with {} profiling (Tracy: {})",
        m_config.enableProfiling ? "enabled" : "disabled",
        m_tracyEnabled ? "enabled" : "disabled"
    );
}

void Profiler::Impl::initializeTracy()
{
#ifdef TRACY_ENABLE
    Logger::debug("Tracy profiling system initialized");
#else
    Logger::warning(
        "Tracy requested but TRACY_ENABLE not defined - profiling disabled"
    );
    m_tracyEnabled = false;
#endif
}

void Profiler::Impl::beginFrame()
{
    if (!m_config.enableProfiling) {
        return;
    }

    m_frameCount++;
    m_currentFrameStart = std::chrono::steady_clock::now();

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        FrameMark;
#endif
    }

    Logger::trace("Frame {} started", m_frameCount);
}

void Profiler::Impl::endFrame()
{
    if (!m_config.enableProfiling) {
        return;
    }

    auto frameEnd = std::chrono::steady_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(
                             frameEnd - m_currentFrameStart
    )
                             .count();

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        FrameMark;
#endif
    }

    Logger::trace("Frame {} ended ({}μs)", m_frameCount, frameDuration);
}

void Profiler::Impl::zone(std::string_view name) const
{
    if (!m_config.enableProfiling) {
        return;
    }

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        ZoneScoped;
        std::string nameStr(name);
        ZoneName(nameStr.c_str(), nameStr.size());
#endif
    }

    Logger::trace("Zone started: {}", name);
}

void Profiler::Impl::zoneValue(std::string_view name, u64 value) const
{
    if (!m_config.enableProfiling) {
        return;
    }

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        ZoneScoped;
        std::string nameStr(name);
        ZoneName(nameStr.c_str(), nameStr.size());
        ZoneValue(value);
#endif
    }

    Logger::trace("Zone started: {} (value: {})", name, value);
}

void Profiler::Impl::zoneText(
    std::string_view name,
    std::string_view text
) const
{
    if (!m_config.enableProfiling) {
        return;
    }

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        ZoneScoped;
        std::string nameStr(name);
        std::string textStr(text);
        ZoneName(nameStr.c_str(), nameStr.size());
        ZoneText(textStr.c_str(), textStr.size());
#endif
    }

    Logger::trace("Zone started: {} (text: {})", name, text);
}

void Profiler::Impl::beginGPUTask(std::string_view name) const
{
    if (!m_config.enableProfiling || !m_config.enableGPUProfiling) {
        return;
    }

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        ZoneScoped;
        std::string nameStr(name);
        ZoneName(nameStr.c_str(), nameStr.size());
#endif
    }

    Logger::trace("GPU task started: {}", name);
}

void Profiler::Impl::endGPUTask() const
{
    if (!m_config.enableProfiling || !m_config.enableGPUProfiling) {
        return;
    }

    Logger::trace("GPU task ended");
}

void Profiler::Impl::beginTextureOperation(
    std::string_view operation,
    std::string_view path
) const
{
    if (!m_config.enableProfiling || !m_config.profileTextureOperations) {
        return;
    }

    auto name = std::format("Texture {}: {}", operation, path);

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        ZoneScoped;
        ZoneName(name.c_str(), name.size());
#endif
    }

    Logger::trace("Texture operation started: {} ({})", operation, path);
}

void Profiler::Impl::endTextureOperation() const
{
    if (!m_config.enableProfiling || !m_config.profileTextureOperations) {
        return;
    }

    Logger::trace("Texture operation ended");
}

void Profiler::Impl::trackAllocation(std::string_view name, size_t size) const
{
    if (!m_config.enableProfiling || !m_config.enableMemoryProfiling) {
        return;
    }

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        std::string nameStr(name);
        TracyAlloc(nameStr.c_str(), size);
#endif
    }

    Logger::trace("Memory allocated: {} ({} bytes)", name, size);
}

void Profiler::Impl::trackDeallocation(std::string_view name, size_t size) const
{
    if (!m_config.enableProfiling || !m_config.enableMemoryProfiling) {
        return;
    }

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        std::string nameStr(name);
        TracyFree(nameStr.c_str());
#endif
    }

    Logger::trace("Memory deallocated: {} ({} bytes)", name, size);
}

void Profiler::Impl::setCounter(std::string_view name, f64 value) const
{
    if (!m_config.enableProfiling) {
        return;
    }

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        std::string nameStr(name);
        TracyPlot(nameStr.c_str(), value);
#endif
    }

    Logger::trace("Counter set: {} = {}", name, value);
}

void Profiler::Impl::plot(std::string_view name, f64 value) const
{
    if (!m_config.enableProfiling) {
        return;
    }

    if (m_tracyEnabled) {
#ifdef TRACY_ENABLE
        std::string nameStr(name);
        TracyPlot(nameStr.c_str(), value);
#endif
    }

    Logger::trace("Plot value: {} = {}", name, value);
}

auto Profiler::init(const ProfilerConfig &config)
    -> std::expected<void, std::string>
{
    try {
        if (g_initialized) {
            return std::unexpected("Profiler already initialized");
        }

        g_config = config;
        g_impl = std::make_unique<Impl>(config);
        g_initialized = true;

        Logger::info("Profiler initialized successfully");
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(
            std::format("Failed to initialize profiler: {}", e.what())
        );
    }
}

void Profiler::shutdown()
{
    if (!g_initialized) {
        return;
    }

    Logger::info("Profiler shutting down");
    g_impl.reset();
    g_initialized = false;
}

auto Profiler::isInitialized() -> bool
{
    return g_initialized && g_impl != nullptr;
}

auto Profiler::getConfig() -> const ProfilerConfig &
{
    return g_config;
}

void Profiler::beginFrame()
{
    if (g_impl) {
        g_impl->beginFrame();
    }
}

void Profiler::endFrame()
{
    if (g_impl) {
        g_impl->endFrame();
    }
}

void Profiler::zone(std::string_view name)
{
    if (g_impl) {
        g_impl->zone(name);
    }
}

void Profiler::zoneValue(std::string_view name, u64 value)
{
    if (g_impl) {
        g_impl->zoneValue(name, value);
    }
}

void Profiler::zoneText(std::string_view name, std::string_view text)
{
    if (g_impl) {
        g_impl->zoneText(name, text);
    }
}

void Profiler::beginGPUTask(std::string_view name)
{
    if (g_impl) {
        g_impl->beginGPUTask(name);
    }
}

void Profiler::endGPUTask()
{
    if (g_impl) {
        g_impl->endGPUTask();
    }
}

void Profiler::beginTextureOperation(
    std::string_view operation,
    std::string_view path
)
{
    if (g_impl) {
        g_impl->beginTextureOperation(operation, path);
    }
}

void Profiler::endTextureOperation()
{
    if (g_impl) {
        g_impl->endTextureOperation();
    }
}

void Profiler::trackAllocation(std::string_view name, size_t size)
{
    if (g_impl) {
        g_impl->trackAllocation(name, size);
    }
}

void Profiler::trackDeallocation(std::string_view name, size_t size)
{
    if (g_impl) {
        g_impl->trackDeallocation(name, size);
    }
}

void Profiler::setCounter(std::string_view name, f64 value)
{
    if (g_impl) {
        g_impl->setCounter(name, value);
    }
}

void Profiler::plot(std::string_view name, f64 value)
{
    if (g_impl) {
        g_impl->plot(name, value);
    }
}

ScopedZone::ScopedZone(std::string_view name)
    : m_name(std::string(name))
{
    Profiler::zone(m_name);
}

ScopedZone::ScopedZone(std::string_view name, u64 value)
    : m_name(std::string(name))
{
    Profiler::zoneValue(m_name, value);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
ScopedZone::ScopedZone(std::string_view name, std::string_view text)
    : m_name(std::string(name))
{
    Profiler::zoneText(m_name, text);
}

ScopedZone::~ScopedZone()
{
    if (m_active) {
        Logger::trace("Zone ended: {}", m_name);
    }
}

ScopedZone::ScopedZone(ScopedZone &&other) noexcept
    : m_name(std::move(other.m_name)),
      m_active(other.m_active)
{
    other.m_active = false;
}

auto ScopedZone::operator=(ScopedZone &&other) noexcept -> ScopedZone &
{
    if (this != &other) {
        m_name = std::move(other.m_name);
        m_active = other.m_active;
        other.m_active = false;
    }
    return *this;
}

ScopedGPUTask::ScopedGPUTask(std::string_view name)
    : m_name(std::string(name))
{
    Profiler::beginGPUTask(m_name);
}

ScopedGPUTask::~ScopedGPUTask()
{
    if (m_active) {
        Profiler::endGPUTask();
    }
}

ScopedGPUTask::ScopedGPUTask(ScopedGPUTask &&other) noexcept
    : m_name(std::move(other.m_name)),
      m_active(other.m_active)
{
    other.m_active = false;
}

auto ScopedGPUTask::operator=(ScopedGPUTask &&other) noexcept -> ScopedGPUTask &
{
    if (this != &other) {
        m_name = std::move(other.m_name);
        m_active = other.m_active;
        other.m_active = false;
    }
    return *this;
}

ScopedTextureOperation::ScopedTextureOperation(
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::string_view operation,
    std::string_view texturePath
)
    : m_operation(std::string(operation)),
      m_texturePath(std::string(texturePath))
{
    Profiler::beginTextureOperation(m_operation, m_texturePath);
}

ScopedTextureOperation::~ScopedTextureOperation()
{
    if (m_active) {
        Profiler::endTextureOperation();
    }
}

ScopedTextureOperation::ScopedTextureOperation(
    ScopedTextureOperation &&other
) noexcept
    : m_operation(std::move(other.m_operation)),
      m_texturePath(std::move(other.m_texturePath)),
      m_active(other.m_active)
{
    other.m_active = false;
}

auto ScopedTextureOperation::operator=(ScopedTextureOperation &&other) noexcept
    -> ScopedTextureOperation &
{
    if (this != &other) {
        m_operation = std::move(other.m_operation);
        m_texturePath = std::move(other.m_texturePath);
        m_active = other.m_active;
        other.m_active = false;
    }
    return *this;
}

} // namespace vostok