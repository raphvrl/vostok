#include "core/logger/logger.hpp"

#include <filesystem>
#include <memory>
#include <mutex>
#include <source_location>
#include <spdlog/async.h>
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
    LogSystem &operator=(const LogSystem &) = delete;
    LogSystem(LogSystem &&) = delete;
    LogSystem &operator=(LogSystem &&) = delete;

    [[nodiscard]] LoggerHandle getLogger(std::string_view name);
    void setDefaultLevel(LogLevel level);
    void setLevel(std::string_view component, LogLevel level);
    void setPattern(std::string_view pattern);
    void setFlushLevel(LogLevel level);
    void flush();
    void enableBacktrace(size_t size);

private:
    std::unordered_map<std::string, std::shared_ptr<LoggerHandle::Impl>> m_loggers;
    std::shared_ptr<spdlog::logger> createSpdLogger(std::string_view name);

    LogConfig m_config;
    std::vector<spdlog::sink_ptr> m_sinks;
    mutable std::mutex m_mutex;

    static spdlog::level::level_enum convertLevel(LogLevel level);
    static LogLevel convertLevel(spdlog::level::level_enum level);
};

class LoggerHandle::Impl
{
public:
    Impl(std::string name, std::shared_ptr<spdlog::logger> logger)
        : m_name(std::move(name)), m_logger(std::move(logger))
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

    [[nodiscard]] bool shouldLog(LogLevel level) const
    {
        return m_logger->should_log(convertLevel(level));
    }

    void setLevel(LogLevel level)
    {
        m_logger->set_level(convertLevel(level));
    }

    [[nodiscard]] LogLevel getLevel() const
    {
        return convertLevel(m_logger->level());
    }

    [[nodiscard]] std::string_view getName() const
    {
        return m_name;
    }

private:
    friend class LogSystem;

    std::string m_name;
    std::shared_ptr<spdlog::logger> m_logger;

    static spdlog::level::level_enum convertLevel(LogLevel level)
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

    static LogLevel convertLevel(spdlog::level::level_enum level)
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

std::unique_ptr<LogSystem> &getLogSystem()
{
    static std::unique_ptr<LogSystem> s_instance;
    return s_instance;
}

std::unordered_map<std::string, std::string> &getContextMap()
{
    static thread_local std::unordered_map<std::string, std::string> s_contextMap;
    return s_contextMap;
}

LoggerHandle &getDefaultLogger()
{
    static LoggerHandle s_defaultLogger(nullptr);
    return s_defaultLogger;
}

LogSystem::LogSystem(const LogConfig &config) : m_config(config)
{
    if (m_config.asyncMode) {
        spdlog::init_thread_pool(8192, 1);
    }

    if (config.console.has_value()) {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(convertLevel(config.level));
        consoleSink->set_pattern(config.pattern);
        m_sinks.push_back(consoleSink);
    }

    if (config.file.has_value()) {
        if (config.file->rotateOnSize) {
            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                config.file->filePath,
                config.file->maxSizeMB * 1024 * 1024,
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

std::shared_ptr<spdlog::logger> LogSystem::createSpdLogger(std::string_view name)
{
    const std::string LOGGER_NAME = !name.empty() ? std::string(name) : "default";
    std::shared_ptr<spdlog::logger> logger;

    if (m_config.asyncMode) {
        logger = std::make_shared<spdlog::async_logger>(
            LOGGER_NAME,
            m_sinks.begin(),
            m_sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block
        );
    } else {
        logger = std::make_shared<spdlog::logger>(LOGGER_NAME, m_sinks.begin(), m_sinks.end());
    }

    spdlog::register_logger(logger);
    return logger;
}

LoggerHandle LogSystem::getLogger(std::string_view name)
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

spdlog::level::level_enum LogSystem::convertLevel(LogLevel level)
{
    return LoggerHandle::Impl::convertLevel(level);
}

LogLevel LogSystem::convertLevel(spdlog::level::level_enum level)
{
    return LoggerHandle::Impl::convertLevel(level);
}

LoggerHandle::LoggerHandle(std::shared_ptr<Impl> impl) : m_impl(std::move(impl)) {}

bool LoggerHandle::shouldLog(LogLevel level) const
{
    return m_impl && m_impl->shouldLog(level);
}

LogLevel LoggerHandle::getLevel() const
{
    return m_impl ? m_impl->getLevel() : LogLevel::OFF;
}

void LoggerHandle::setLevel(LogLevel level)
{
    if (m_impl) {
        m_impl->setLevel(level);
    }
}

std::string_view LoggerHandle::getName() const
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

std::expected<void, std::string> Logger::init(const LogConfig &config)
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

LoggerHandle Logger::getLogger(std::string_view name)
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

void Logger::binary(std::string_view name, const void *data, size_t size)
{
    if (!getLogSystem() || data == nullptr || size == 0) {
        return;
    }

    const byte *byteData = static_cast<const byte *>(data);
    constexpr size_t BYTES_PER_ROW = 16;

    getDefaultLogger().debug("Binary data: {} ({} bytes)", name, size);

    for (size_t i = 0; i < size; i += BYTES_PER_ROW) {
        std::string line = std::format("{:08X}: ", i);
        std::string ascii;

        for (size_t j = 0; j < BYTES_PER_ROW && (i + j) < size; ++j) {
            byte b = byteData[i + j];
            line += std::format("{:02X} ", b);
            ascii += (b >= 32 && b < 127) ? static_cast<char>(b) : '.';
        }

        size_t padding = (BYTES_PER_ROW - ((size - i) % BYTES_PER_ROW)) % BYTES_PER_ROW;
        line += std::string(padding * 3, ' ');
        line += "| " + ascii;

        getDefaultLogger().debug("{}", line);
    }
}

LoggerHandle &Logger::getDefaultLogger()
{
    static LoggerHandle s_defaultLogger(nullptr);

    if (!getLogSystem()) {
        auto result = init();
        if (!result) {
            return s_defaultLogger;
        }
        // Réinitialisation du logger par défaut après initialisation
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
    : m_name(name), m_level(level), m_start(std::chrono::high_resolution_clock::now())
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
