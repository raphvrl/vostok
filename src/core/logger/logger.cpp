#include "core/logger/logger.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <mutex>
#include <source_location>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace vostok
{

class LogSystem
{
public:
    explicit LogSystem(const LogConfig &config);
    ~LogSystem();

    LogSystem(const LogSystem &) = delete;
    auto operator=(const LogSystem &) -> LogSystem & = delete;
    LogSystem(LogSystem &&) = delete;
    auto operator=(LogSystem &&) -> LogSystem & = delete;

    [[nodiscard]] auto getLogger(std::string_view name) -> LoggerHandle;
    void setDefaultLevel(LogLevel level);
    void setLevel(std::string_view component, LogLevel level);
    void setPattern(std::string_view pattern);
    void setFlushLevel(LogLevel level);
    void flush();
    void enableBacktrace(size_t size);

private:
    std::unordered_map<std::string, std::shared_ptr<LoggerHandle::Impl>> m_loggers;
    auto createSpdLogger(std::string_view name) -> std::shared_ptr<spdlog::logger>;
    static auto formatLogFilename(const std::string &pattern, std::string_view loggerName)
        -> std::string;

    LogConfig m_config;
    std::vector<spdlog::sink_ptr> m_sinks;
    mutable std::mutex m_mutex;

    static auto convertLevel(LogLevel level) -> spdlog::level::level_enum;
    static auto convertLevel(spdlog::level::level_enum level) -> LogLevel;
};

class LoggerHandle::Impl
{
public:
    Impl(std::string name, std::shared_ptr<spdlog::logger> logger)
        : m_name(std::move(name)),
          m_logger(std::move(logger))
    {}

    void log(LogLevel level, std::string_view message, const std::source_location &location)
    {
        const auto FILE = fs::path(location.file_name()).filename().string();
        const auto LOC_INFO = std::format("{}:{}", FILE, location.line());

        switch (level) {
            case LogLevel::TRACE:
                m_logger->trace("[{}] {}", LOC_INFO, message);
                break;
            case LogLevel::DEBUG:
                m_logger->debug("[{}] {}", LOC_INFO, message);
                break;
            case LogLevel::INFO:
                m_logger->info("[{}] {}", LOC_INFO, message);
                break;
            case LogLevel::WARNING:
                m_logger->warn("[{}] {}", LOC_INFO, message);
                break;
            case LogLevel::ERROR:
                m_logger->error("[{}] {}", LOC_INFO, message);
                break;
            case LogLevel::CRITICAL:
                m_logger->critical("[{}] {}", LOC_INFO, message);
                break;
            case LogLevel::OFF:
                break; // No logging for OFF level
        }
    }

    [[nodiscard]] auto shouldLog(LogLevel level) const -> bool
    {
        return m_logger->should_log(convertLevel(level));
    }

    void setLevel(LogLevel level) { m_logger->set_level(convertLevel(level)); }

    [[nodiscard]] auto getLevel() const -> LogLevel { return convertLevel(m_logger->level()); }

    [[nodiscard]] auto getName() const -> std::string_view { return m_name; }

private:
    friend class LogSystem;

    std::string m_name;
    std::shared_ptr<spdlog::logger> m_logger;

    static auto convertLevel(LogLevel level) -> spdlog::level::level_enum
    {
        switch (level) {
            case LogLevel::TRACE:
                return spdlog::level::trace;
            case LogLevel::DEBUG:
                return spdlog::level::debug;
            case LogLevel::INFO:
                return spdlog::level::info;
            case LogLevel::WARNING:
                return spdlog::level::warn;
            case LogLevel::ERROR:
                return spdlog::level::err;
            case LogLevel::CRITICAL:
                return spdlog::level::critical;
            case LogLevel::OFF:
                return spdlog::level::off;
        }
        return spdlog::level::info;
    }

    static auto convertLevel(spdlog::level::level_enum level) -> LogLevel
    {
        if (level == spdlog::level::trace) {
            return LogLevel::TRACE;
        }
        if (level == spdlog::level::debug) {
            return LogLevel::DEBUG;
        }
        if (level == spdlog::level::info) {
            return LogLevel::INFO;
        }
        if (level == spdlog::level::warn) {
            return LogLevel::WARNING;
        }
        if (level == spdlog::level::err) {
            return LogLevel::ERROR;
        }
        if (level == spdlog::level::critical) {
            return LogLevel::CRITICAL;
        }
        if (level == spdlog::level::off) {
            return LogLevel::OFF;
        }

        return LogLevel::INFO;
    };
};

auto getLogSystem() -> std::unique_ptr<LogSystem> &
{
    static std::unique_ptr<LogSystem> s_instance;
    return s_instance;
}

auto getContextMap() -> std::unordered_map<std::string, std::string> &
{
    static thread_local std::unordered_map<std::string, std::string> s_contextMap;
    return s_contextMap;
}

auto getDefaultLogger() -> LoggerHandle &
{
    static LoggerHandle s_defaultLogger(nullptr);
    return s_defaultLogger;
}

LogSystem::LogSystem(const LogConfig &config)
    : m_config(config)
{
    if (m_config.asyncMode) {
        constexpr size_t THREAD_POOL_QUEUE_SIZE = 8192;
        constexpr size_t THREAD_POOL_THREADS = 1;
        spdlog::init_thread_pool(THREAD_POOL_QUEUE_SIZE, THREAD_POOL_THREADS);
    }

    if (config.console.has_value()) {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(convertLevel(config.level));
        consoleSink->set_pattern(config.pattern);
        m_sinks.push_back(consoleSink);
    }

    if (config.file.has_value() && (!config.file->separateFilesByComponent ||
                                    config.file->filePath != config.file->componentFilePattern)) {
        if (config.file->rotateOnSize) {
            constexpr size_t BYTES_PER_MB = 1024 * 1024;
            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                config.file->filePath,
                config.file->maxSizeMB * BYTES_PER_MB,
                config.file->maxFiles,
                config.file->truncateOnStart
            );
            fileSink->set_level(convertLevel(config.level));
            fileSink->set_pattern(config.pattern);
            m_sinks.push_back(fileSink);
        } else {
            auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                config.file->filePath,
                config.file->truncateOnStart
            );
            fileSink->set_level(convertLevel(config.level));
            fileSink->set_pattern(config.pattern);
            m_sinks.push_back(fileSink);
        }
    }

    spdlog::flush_on(convertLevel(config.flushLevel));

    auto defaultLogger = createSpdLogger("");
    defaultLogger->set_level(convertLevel(config.level));

    for (const auto &[component, level] : config.componentLevels) {
        auto logger = createSpdLogger(component);
        logger->set_level(convertLevel(level));
    }
}

LogSystem::~LogSystem()
{
    flush();
    spdlog::shutdown();
}

auto LogSystem::createSpdLogger(std::string_view name) -> std::shared_ptr<spdlog::logger>
{
    const std::string LOGGER_NAME = !name.empty() ? std::string(name) : "default";
    std::shared_ptr<spdlog::logger> logger;

    std::vector<spdlog::sink_ptr> sinks;

    if (m_config.console.has_value()) {
        for (auto &sink : m_sinks) {
            if (dynamic_cast<spdlog::sinks::stdout_color_sink_mt *>(sink.get()) != nullptr) {
                sinks.push_back(sink);
            }
        }
    }

    if (m_config.file.has_value() && m_config.file->separateFilesByComponent) {
        std::string filename;

        if (name.empty()) {
            filename = m_config.file->filePath;
        } else {
            filename = formatLogFilename(m_config.file->componentFilePattern, name);
            size_t pos = filename.find("{name}");
            if (pos != std::string::npos) {
                filename.replace(pos, 6, std::string(name));
            }
        }

        spdlog::sink_ptr fileSink;
        if (m_config.file->rotateOnSize) {
            constexpr size_t BYTES_PER_MB = 1024 * 1024;
            fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                filename,
                m_config.file->maxSizeMB * BYTES_PER_MB,
                m_config.file->maxFiles,
                m_config.file->truncateOnStart
            );
        } else {
            fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                filename,
                m_config.file->truncateOnStart
            );
        }
        fileSink->set_level(convertLevel(m_config.level));
        fileSink->set_pattern(m_config.pattern);
        sinks.push_back(fileSink);
    } else {
        for (const auto &sink : m_sinks) {
            if (dynamic_cast<spdlog::sinks::basic_file_sink_mt *>(sink.get()) != nullptr ||
                dynamic_cast<spdlog::sinks::rotating_file_sink_mt *>(sink.get()) != nullptr) {
                sinks.push_back(sink);
            }
        }
    }

    if (m_config.asyncMode) {
        logger = std::make_shared<spdlog::async_logger>(
            LOGGER_NAME,
            sinks.begin(),
            sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block
        );
    } else {
        logger = std::make_shared<spdlog::logger>(LOGGER_NAME, sinks.begin(), sinks.end());
    }

    spdlog::register_logger(logger);
    return logger;
}

auto LogSystem::formatLogFilename(const std::string &pattern, std::string_view loggerName)
    -> std::string
{
    std::string result = pattern;

    size_t pos = result.find("{name}");
    if (pos != std::string::npos) {
        std::string name = loggerName.empty() ? "default" : std::string(loggerName);

        std::ranges::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        result.replace(pos, 6, name);
    }

    return result;
}

auto LogSystem::getLogger(std::string_view name) -> LoggerHandle
{
    const std::string KEY = !name.empty() ? std::string(name) : "default";

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_loggers.find(KEY);
    if (it != m_loggers.end()) {
        return { it->second };
    }

    auto spdLogger = spdlog::get(KEY);
    if (!spdLogger) {
        spdLogger = createSpdLogger(name);

        auto levelIt = m_config.componentLevels.find(KEY);
        if (levelIt != m_config.componentLevels.end()) {
            spdLogger->set_level(convertLevel(levelIt->second));
        } else {
            spdLogger->set_level(convertLevel(m_config.level));
        }
    }
    auto impl = std::make_shared<LoggerHandle::Impl>(KEY, spdLogger);
    m_loggers[KEY] = impl;

    return { impl };
}

void LogSystem::setDefaultLevel(LogLevel level)
{
    for (const auto &sink : m_sinks) {
        sink->set_level(convertLevel(level));
    }

    auto defaultLogger = spdlog::get("default");
    if (defaultLogger) {
        defaultLogger->set_level(convertLevel(level));
    }
}

void LogSystem::setLevel(std::string_view component, LogLevel level)
{
    const std::string KEY = std::string(component);
    auto logger = spdlog::get(KEY);

    if (logger) {
        logger->set_level(convertLevel(level));
    }

    m_config.componentLevels[KEY] = level;
}

void LogSystem::setPattern(std::string_view pattern)
{
    const std::string PATTERN_STR = std::string(pattern);

    for (const auto &sink : m_sinks) {
        sink->set_pattern(PATTERN_STR);
    }

    m_config.pattern = PATTERN_STR;
}

void LogSystem::setFlushLevel(LogLevel level)
{
    spdlog::flush_on(convertLevel(level));
    m_config.flushLevel = level;
}

void LogSystem::flush()
{
    for (const auto &sink : m_sinks) {
        sink->flush();
    }
}

void LogSystem::enableBacktrace(size_t size)
{
    spdlog::enable_backtrace(size);
    m_config.captureBacktrace = true;
    m_config.backtraceSize = size;
}

auto LogSystem::convertLevel(LogLevel level) -> spdlog::level::level_enum
{
    return LoggerHandle::Impl::convertLevel(level);
}

auto LogSystem::convertLevel(spdlog::level::level_enum level) -> LogLevel
{
    return LoggerHandle::Impl::convertLevel(level);
}

LoggerHandle::LoggerHandle(std::shared_ptr<Impl> impl)
    : m_impl(std::move(impl))
{}

auto LoggerHandle::shouldLog(LogLevel level) const -> bool
{
    return m_impl && m_impl->shouldLog(level);
}

auto LoggerHandle::getLevel() const -> LogLevel
{
    return m_impl ? m_impl->getLevel() : LogLevel::OFF;
}

void LoggerHandle::setLevel(LogLevel level)
{
    if (m_impl) {
        m_impl->setLevel(level);
    }
}

auto LoggerHandle::getName() const -> std::string_view
{
    static const std::string EMPTY;
    return m_impl ? m_impl->getName() : EMPTY;
}

void LoggerHandle::logImpl(
    LogLevel level,
    std::string_view message,
    const std::source_location &location
) const
{
    if (m_impl) {
        m_impl->log(level, message, location);
    }
}

auto Logger::init(const LogConfig &config) -> std::expected<void, std::string>
{
    try {
        getLogSystem() = std::make_unique<LogSystem>(config);
        getDefaultLogger() = getLogSystem()->getLogger("");
        return {};
    } catch (const std::exception &e) {
        return std::unexpected(e.what());
    }
}

void Logger::shutdown()
{
    getDefaultLogger() = LoggerHandle{ nullptr };
    getLogSystem().reset();
}

auto Logger::getLogger(std::string_view name) -> LoggerHandle
{
    if (!getLogSystem()) {
        auto result = init();
        if (!result) {
            return LoggerHandle{ nullptr };
        }
    }

    return getLogSystem()->getLogger(name);
}

void Logger::setDefaultLevel(LogLevel level)
{
    if (getLogSystem()) {
        getLogSystem()->setDefaultLevel(level);
    }
}

void Logger::setLevel(std::string_view component, LogLevel level)
{
    if (getLogSystem()) {
        getLogSystem()->setLevel(component, level);
    }
}

void Logger::setPattern(std::string_view pattern)
{
    if (getLogSystem()) {
        getLogSystem()->setPattern(pattern);
    }
}

void Logger::flushOn(LogLevel level)
{
    if (getLogSystem()) {
        getLogSystem()->setFlushLevel(level);
    }
}

void Logger::flush()
{
    if (getLogSystem()) {
        getLogSystem()->flush();
    }
}

void Logger::binary(std::string_view name, std::span<const byte> data)
{
    if (!getLogSystem() || data.empty()) {
        return;
    }

    constexpr size_t BYTES_PER_ROW = 16;

    getDefaultLogger().debug("Binary data: {} ({} bytes)", name, data.size());

    for (size_t i = 0; i < data.size(); i += BYTES_PER_ROW) {
        auto lineSpan = data.subspan(i, std::min(BYTES_PER_ROW, data.size() - i));

        std::string line = std::format("{:08X}: ", i);
        std::string ascii;

        for (const auto &b : lineSpan) {
            line += std::format("{:02X} ", b);
            ascii += (b >= 32 && b < 127) ? static_cast<char>(b) : '.';
        }

        size_t padding = BYTES_PER_ROW - lineSpan.size();
        line += std::string(padding * 3, ' ');
        line += "| " + ascii;

        getDefaultLogger().debug("{}", line);
    }
}

auto Logger::getDefaultLogger() -> LoggerHandle &
{
    static LoggerHandle s_defaultLogger(nullptr);

    if (!getLogSystem()) {
        auto result = init();
        if (!result) {
            return s_defaultLogger;
        }

        s_defaultLogger = getLogSystem()->getLogger("");
    }

    return s_defaultLogger;
}

ScopedContext::ScopedContext(std::pair<std::string_view, std::string_view> keyValue)
    : m_key(keyValue.first)
{
    getContextMap()[m_key] = std::string(keyValue.second);
}

ScopedContext::~ScopedContext()
{
    getContextMap().erase(m_key);
}

ScopedTimer::ScopedTimer(std::string_view name, LogLevel level)
    : m_name(name),
      m_level(level),
      m_start(std::chrono::high_resolution_clock::now())
{}

ScopedTimer::~ScopedTimer() noexcept
{
    try {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();

        switch (m_level) {
            case LogLevel::TRACE:
                Logger::trace("{} completed in {} ms", m_name, duration);
                break;
            case LogLevel::DEBUG:
                Logger::debug("{} completed in {} ms", m_name, duration);
                break;
            case LogLevel::INFO:
                Logger::info("{} completed in {} ms", m_name, duration);
                break;
            case LogLevel::WARNING:
                Logger::warning("{} completed in {} ms", m_name, duration);
                break;
            case LogLevel::ERROR:
                Logger::error("{} completed in {} ms", m_name, duration);
                break;
            case LogLevel::CRITICAL:
                Logger::critical("{} completed in {} ms", m_name, duration);
                break;
            case LogLevel::OFF:
                break; // No logging for OFF level
        }
    } catch (const std::exception &e) {
        fputs("Exception in ScopedTimer: ", stderr);
        fputs(e.what(), stderr);
        fputs("\n", stderr);
    } catch (...) {
        fputs("Unknown exception in ScopedTimer\n", stderr);
    }
}

} // namespace vostok
