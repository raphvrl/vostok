#pragma once

#include "vostok/core/logger/logger.hpp"

namespace vostok
{

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void LoggerHandle::trace(const FormatWithLocation &fmtLoc, Args &&...args) const
{
    if (!shouldLog(LogLevel::TRACE)) {
        return;
    }

    std::string message = fmtLoc.format(std::forward<Args>(args)...);
    logImpl(LogLevel::TRACE, message, fmtLoc.location);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void LoggerHandle::debug(const FormatWithLocation &fmtLoc, Args &&...args) const
{
    if (!shouldLog(LogLevel::DEBUG)) {
        return;
    }

    std::string message = fmtLoc.format(std::forward<Args>(args)...);
    logImpl(LogLevel::DEBUG, message, fmtLoc.location);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void LoggerHandle::info(const FormatWithLocation &fmtLoc, Args &&...args) const
{
    if (!shouldLog(LogLevel::INFO)) {
        return;
    }

    std::string message = fmtLoc.format(std::forward<Args>(args)...);
    logImpl(LogLevel::INFO, message, fmtLoc.location);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void LoggerHandle::warning(const FormatWithLocation &fmtLoc, Args &&...args) const
{
    if (!shouldLog(LogLevel::WARNING)) {
        return;
    }

    std::string message = fmtLoc.format(std::forward<Args>(args)...);
    logImpl(LogLevel::WARNING, message, fmtLoc.location);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void LoggerHandle::error(const FormatWithLocation &fmtLoc, Args &&...args) const
{
    if (!shouldLog(LogLevel::ERROR)) {
        return;
    }

    std::string message = fmtLoc.format(std::forward<Args>(args)...);
    logImpl(LogLevel::ERROR, message, fmtLoc.location);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void LoggerHandle::critical(const FormatWithLocation &fmtLoc, Args &&...args) const
{
    if (!shouldLog(LogLevel::CRITICAL)) {
        return;
    }

    std::string message = fmtLoc.format(std::forward<Args>(args)...);
    logImpl(LogLevel::CRITICAL, message, fmtLoc.location);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void Logger::trace(const FormatWithLocation &fmtLoc, Args &&...args)
{
    getDefaultLogger().trace(fmtLoc, std::forward<Args>(args)...);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void Logger::debug(const FormatWithLocation &fmtLoc, Args &&...args)
{
    getDefaultLogger().debug(fmtLoc, std::forward<Args>(args)...);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void Logger::info(const FormatWithLocation &fmtLoc, Args &&...args)
{
    getDefaultLogger().info(fmtLoc, std::forward<Args>(args)...);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void Logger::warning(const FormatWithLocation &fmtLoc, Args &&...args)
{
    getDefaultLogger().warning(fmtLoc, std::forward<Args>(args)...);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void Logger::error(const FormatWithLocation &fmtLoc, Args &&...args)
{
    getDefaultLogger().error(fmtLoc, std::forward<Args>(args)...);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void Logger::critical(const FormatWithLocation &fmtLoc, Args &&...args)
{
    getDefaultLogger().critical(fmtLoc, std::forward<Args>(args)...);
}

template <typename... Args>
    requires(std::formattable<Args, char> && ...)
void Logger::gpu(const FormatWithLocation &fmtLoc, Args &&...args)
{
    auto &logger = getDefaultLogger();

    std::string formattedMsg = fmtLoc.format(std::forward<Args>(args)...);
    auto gpuFmtLoc = FormatWithLocation("[GPU] {}", fmtLoc.location);

    logger.info(gpuFmtLoc, formattedMsg);
}

} // namespace vostok