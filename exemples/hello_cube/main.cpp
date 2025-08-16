#include "app.hpp"
#include "vostok/core/logger/logger.hpp"

#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

auto getExecutablePath() -> fs::path
{
#ifdef _WIN32
    std::array<wchar_t, MAX_PATH> path{};
    GetModuleFileNameW(nullptr, static_cast<LPWSTR>(path.data()), MAX_PATH);
    return fs::path(path.data()).parent_path();
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return fs::path(std::string(result, (count > 0) ? count : 0)).parent_path();
#endif
}

auto initializeLogger() -> bool
{
    fs::path executablePath = getExecutablePath();
    fs::path logDir = executablePath / "logs";

    try {
        if (!fs::exists(logDir)) {
            fs::create_directories(logDir);
        }
    } catch (const std::exception &e) {
        Logger::error("Failed to create log directory: {}", e.what());
        logDir = fs::current_path() / "logs";
        try {
            if (!fs::exists(logDir)) {
                fs::create_directories(logDir);
            }
        } catch (const std::exception &e) {
            Logger::error(
                "Failed to create log directory in working path: {}",
                e.what()
            );
            logDir = fs::current_path();
        }
    }
    LogConfig logConfig;
    logConfig.asyncMode = false;
    logConfig.level = LogLevel::DEBUG;
    logConfig.file = FileLogConfig();
    logConfig.file->filePath = (logDir / "vostok_default.log").string();
    logConfig.file->truncateOnStart = true;
    logConfig.file->separateFilesByComponent = true;
    logConfig.file->componentFilePattern =
        (logDir / "vostok_{name}.log").string();

    auto result = Logger::init(logConfig);
    if (!result) {
        Logger::error("Failed to initialize logger: {}", result.error());
        return false;
    }

    Logger::info("Logger initialized");
    Logger::info("Logs will be stored in: {}", logDir.string());
    return true;
}

auto main(int argc, char *argv[]) -> int
{
    if (!initializeLogger()) {
        return -1;
    }

    Logger::info("Vostok App starting");
    Logger::info("Working directory: {}", fs::current_path().string());

    try {
        App app;

        if (!app.initialize()) {
            Logger::error("Failed to initialize application");
            return -1;
        }

        app.run();
        app.shutdown();

        Logger::info("Application exited successfully");

    } catch (const std::exception &e) {
        Logger::critical("Unhandled exception: {}", e.what());
        return -1;
    }

    Logger::info("Shutting down logger");
    Logger::shutdown();
    return 0;
}