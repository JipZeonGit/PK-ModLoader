#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <windows.h>

namespace Logger {
    enum Level {
        LEVEL_DEBUG,
        LEVEL_INFO,
        LEVEL_WARN,
        LEVEL_ERROR
    };

    void Initialize(bool openConsole = false, const std::string& logFilename = "");
    void Shutdown();

    void Log(Level level, const char* format, ...);

    #define LOG_DEBUG(fmt, ...) Logger::Log(Logger::LEVEL_DEBUG, fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...)  Logger::Log(Logger::LEVEL_INFO,  fmt, ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...)  Logger::Log(Logger::LEVEL_WARN,  fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...) Logger::Log(Logger::LEVEL_ERROR, fmt, ##__VA_ARGS__)
}
