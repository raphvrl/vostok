#pragma once

#include "vostok/core/type.hpp"

#include <chrono>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <unordered_map>

namespace vostok
{

enum class LogLevel : u8
{
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
    OFF
};

constexpr std::string_view toString(LogLevel level)
{
    switch (level) {
        case LogLevel::TRACE:
            return "TRACE";
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        case LogLevel::OFF:
            return "OFF";
    }
}

struct FileLogConfig
{
    std::string filePath = "vostok.log";
    bool rotateOnSize = false;
    size_t maxSizeMB = 10;
    size_t maxFiles = 5;
    bool truncateOnStart = true;

    bool separateFilesByComponent = false;
    std::string componentFilePattern = "vostok_{name}.log";
};

struct ConsoleLogConfig
{
    bool enableColors = true;
    bool useStdErr = false;
};

struct LogConfig
{
    LogLevel level = LogLevel::INFO;

    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v";

    std::optional<ConsoleLogConfig> console;
    std::optional<FileLogConfig> file;

    LogLevel flushLevel = LogLevel::ERROR;

    std::unordered_map<std::string, LogLevel> componentLevels;

    bool asyncMode = true;
    bool captureBacktrace = false;
    size_t backtraceSize = 10;
};

struct FormatWithLocation
{
    std::string_view fmt;
    std::source_location location;

    FormatWithLocation(
        std::string_view f,
        const std::source_location &loc = std::source_location::current()
    )
        : fmt(f), location(loc)
    {}

    FormatWithLocation(
        const char *f,
        const std::source_location &loc = std::source_location::current()
    )
        : fmt(f), location(loc)
    {}

    template <typename T>
    static FormatWithLocation
    at(T &&fmt, const std::source_location &loc = std::source_location::current())
    {
        return FormatWithLocation(std::forward<T>(fmt), loc);
    }

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    [[nodiscard]] std::string format(Args &&...args) const
    {
        try {
            auto argsTuple = std::forward_as_tuple(std::forward<Args>(args)...);

            return std::apply(
                [this](auto &&...a) {
                    return std::vformat(std::string(fmt), std::make_format_args(a...));
                },
                argsTuple
            );
        } catch (const std::format_error &e) {
            return std::string(fmt) + " [Format error]";
        }
    }
};

class LoggerHandle
{
public:
    [[nodiscard]] bool shouldLog(LogLevel level) const;

    [[nodiscard]] LogLevel getLevel() const;
    void setLevel(LogLevel level);

    [[nodiscard]] std::string_view getName() const;

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    void trace(const FormatWithLocation &fmtLoc, Args &&...args) const;

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    void debug(const FormatWithLocation &fmtLoc, Args &&...args) const;

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    void info(const FormatWithLocation &fmtLoc, Args &&...args) const;

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    void warning(const FormatWithLocation &fmtLoc, Args &&...args) const;

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    void error(const FormatWithLocation &fmtLoc, Args &&...args) const;

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    void critical(const FormatWithLocation &fmtLoc, Args &&...args) const;

private:
    class Impl;
    std::shared_ptr<Impl> m_impl;

    LoggerHandle(std::shared_ptr<Impl> impl);

    friend class Logger;
    friend class LogSystem;
    friend LoggerHandle &getDefaultLogger();

    void
    logImpl(LogLevel level, std::string_view message, const std::source_location &location) const;
};

class Logger
{
public:
    static std::expected<void, std::string> init(const LogConfig &config = {});
    static void shutdown();

    static LoggerHandle getLogger(std::string_view name);

    static void setDefaultLevel(LogLevel level);
    static void setLevel(std::string_view component, LogLevel level);
    static void setPattern(std::string_view pattern);
    static void flushOn(LogLevel level);
    static void flush();

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    static void trace(const FormatWithLocation &fmtLoc, Args &&...args);

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    static void debug(const FormatWithLocation &fmtLoc, Args &&...args);

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    static void info(const FormatWithLocation &fmtLoc, Args &&...args);

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    static void warning(const FormatWithLocation &fmtLoc, Args &&...args);

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    static void error(const FormatWithLocation &fmtLoc, Args &&...args);

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    static void critical(const FormatWithLocation &fmtLoc, Args &&...args);

    static void binary(std::string_view name, const void *data, size_t size);

    template <typename... Args>
        requires(std::formattable<Args, char> && ...)
    static void gpu(const FormatWithLocation &fmtLoc, Args &&...args);

private:
    Logger() = default;

    static LoggerHandle &getDefaultLogger();
};

class ScopedContext
{
public:
    ScopedContext(std::pair<std::string_view, std::string_view> keyValue);
    ~ScopedContext();

    ScopedContext(const ScopedContext &) = delete;
    ScopedContext &operator=(const ScopedContext &) = delete;
    ScopedContext(ScopedContext &&) = delete;
    ScopedContext &operator=(ScopedContext &&) = delete;

private:
    std::string m_key;
};

class ScopedTimer
{
public:
    explicit ScopedTimer(std::string_view name, LogLevel level = LogLevel::DEBUG);
    ~ScopedTimer() noexcept;

    ScopedTimer(const ScopedTimer &) = delete;
    ScopedTimer &operator=(const ScopedTimer &) = delete;
    ScopedTimer(ScopedTimer &&) = delete;
    ScopedTimer &operator=(ScopedTimer &&) = delete;

private:
    std::string m_name;
    LogLevel m_level;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

} // namespace vostok

#include "vostok/core/logger/logger.inl"

template <>
struct std::formatter<vostok::LogLevel> : std::formatter<std::string_view>
{
    auto format(vostok::LogLevel level, format_context &ctx) const
    {
        return formatter<std::string_view>::format(vostok::toString(level), ctx);
    }
};