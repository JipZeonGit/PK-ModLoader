#include "logger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <string>

namespace Logger {

namespace {
    std::ofstream g_logFile;
    std::mutex g_logMutex;
    bool g_hasConsole = false;
    HANDLE g_hConsoleOut = INVALID_HANDLE_VALUE;

    std::string GetGameDirectoryA() {
        char path[MAX_PATH] = {0};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string s(path);
        size_t pos = s.find_last_of("\\/");
        return (pos != std::string::npos) ? s.substr(0, pos) : ".";
    }
}

void Initialize(bool openConsole, const std::string& consoleTitle, const std::string& logFilename) {
    std::lock_guard<std::mutex> lock(g_logMutex);

    if (openConsole) {
        if (AllocConsole()) {
            FILE* fp;
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
            SetConsoleTitleA(consoleTitle.c_str());
            g_hasConsole = true;
            g_hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
        } else {
            g_hasConsole = true;
            g_hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
        }
    }

    std::string gameDir = GetGameDirectoryA();
    std::string modsDir = gameDir + "\\mods";
    std::string logsDir = modsDir + "\\logs";
    CreateDirectoryA(modsDir.c_str(), nullptr);
    CreateDirectoryA(logsDir.c_str(), nullptr);

    std::string fullLogPath = logFilename;
    if (logFilename.find(':') == std::string::npos && logFilename.rfind("\\\\", 0) != 0) {
        fullLogPath = gameDir + "\\" + logFilename;
    }

    g_logFile.open(fullLogPath, std::ios::out | std::ios::trunc);
    if (g_logFile.is_open()) {
        g_logFile << "=========================================================\n";
        g_logFile << "[Init] Portal Knights Native Mod Loader Logger Initialized.\n";
        g_logFile << "[Init] Log file location: " << fullLogPath << "\n";
        g_logFile << "=========================================================\n";
        g_logFile.flush();
    }
}

void Shutdown() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile.is_open()) {
        g_logFile << "[Shutdown] Mod loader logger shutting down.\n";
        g_logFile.flush();
        g_logFile.close();
    }
    if (g_hasConsole) {
        FreeConsole();
        g_hasConsole = false;
        g_hConsoleOut = INVALID_HANDLE_VALUE;
    }
}

void Log(Level level, const char* format, ...) {
    std::lock_guard<std::mutex> lock(g_logMutex);

    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    const char* levelStr = "INFO";
    switch (level) {
        case LEVEL_DEBUG: levelStr = "DEBUG"; break;
        case LEVEL_INFO:  levelStr = "INFO "; break;
        case LEVEL_WARN:  levelStr = "WARN "; break;
        case LEVEL_ERROR: levelStr = "ERROR"; break;
    }

    time_t now = time(nullptr);
    tm localTm;
    localtime_s(&localTm, &now);

    char timeBuf[32];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &localTm);

    char finalMsg[2200];
    snprintf(finalMsg, sizeof(finalMsg), "[%s][%s] %s\n", timeBuf, levelStr, buffer);

    if (g_hConsoleOut != INVALID_HANDLE_VALUE && g_hConsoleOut != nullptr) {
        DWORD written = 0;
        WriteConsoleA(g_hConsoleOut, finalMsg, (DWORD)strlen(finalMsg), &written, nullptr);
    } else if (g_hasConsole) {
        printf("%s", finalMsg);
    }

    if (g_logFile.is_open()) {
        g_logFile << finalMsg;
        g_logFile.flush();
    }

    OutputDebugStringA(finalMsg);
}

} // namespace Logger
