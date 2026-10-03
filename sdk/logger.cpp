#include "logger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <string>

namespace Logger {

namespace {
    std::ofstream g_logFile;
    std::mutex g_logMutex;
    bool g_initialized = false;
    HANDLE g_hConsoleOut = INVALID_HANDLE_VALUE;

    std::string GetGameDirectoryA() {
        char path[MAX_PATH] = {0};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string s(path);
        size_t pos = s.find_last_of("\\/");
        return (pos != std::string::npos) ? s.substr(0, pos) : ".";
    }

    std::string GetCurrentDllBaseName() {
        char path[MAX_PATH] = {0};
        HMODULE hMod = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&GetCurrentDllBaseName),
            &hMod
        );
        GetModuleFileNameA(hMod, path, MAX_PATH);
        std::string s(path);
        size_t pos = s.find_last_of("\\/");
        std::string name = (pos != std::string::npos) ? s.substr(pos + 1) : "Plugin";
        size_t dot = name.find_last_of('.');
        if (dot != std::string::npos) {
            name = name.substr(0, dot);
        }
        return name;
    }

    void EnsureInitializedUnlocked(bool openConsole = false, const std::string& customFilename = "") {
        if (g_initialized) return;
        g_initialized = true;

        // Check if there is an active console window or handle
        HWND hConsoleWnd = GetConsoleWindow();
        if (hConsoleWnd != nullptr) {
            g_hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
        } else if (openConsole) {
            if (AllocConsole()) {
                FILE* fp;
                freopen_s(&fp, "CONOUT$", "w", stdout);
                freopen_s(&fp, "CONOUT$", "w", stderr);
                g_hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
            }
        }

        std::string gameDir = GetGameDirectoryA();
        std::string modsDir = gameDir + "\\mods";
        std::string logsDir = modsDir + "\\logs";
        CreateDirectoryA(modsDir.c_str(), nullptr);
        CreateDirectoryA(logsDir.c_str(), nullptr);

        std::string fullPath;
        if (!customFilename.empty()) {
            if (customFilename.find(':') != std::string::npos || customFilename.rfind("\\\\", 0) == 0) {
                fullPath = customFilename;
            } else {
                fullPath = logsDir + "\\" + customFilename;
            }
        } else {
            std::string dllName = GetCurrentDllBaseName();
            fullPath = logsDir + "\\" + dllName + ".log";
        }

        g_logFile.open(fullPath, std::ios::out | std::ios::app);
        if (g_logFile.is_open()) {
            g_logFile << "\n=========================================================\n";
            g_logFile << "[Init] Plugin Logger Initialized for: " << GetCurrentDllBaseName() << "\n";
            g_logFile << "[Init] Log file location: " << fullPath << "\n";
            g_logFile << "=========================================================\n";
            g_logFile.flush();
        }
    }
}

void Initialize(bool openConsole, const std::string& logFilename) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    EnsureInitializedUnlocked(openConsole, logFilename);
}

void Shutdown() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile.is_open()) {
        g_logFile << "[Shutdown] Plugin logger shutting down.\n";
        g_logFile.flush();
        g_logFile.close();
    }
    g_initialized = false;
    g_hConsoleOut = INVALID_HANDLE_VALUE;
}

void Log(Level level, const char* format, ...) {
    std::lock_guard<std::mutex> lock(g_logMutex);

    if (!g_initialized) {
        EnsureInitializedUnlocked();
    }

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

    // 1. Output directly to Console via Win32 API
    if (g_hConsoleOut != INVALID_HANDLE_VALUE && g_hConsoleOut != nullptr) {
        DWORD written = 0;
        WriteConsoleA(g_hConsoleOut, finalMsg, (DWORD)strlen(finalMsg), &written, nullptr);
    } else {
        HANDLE hCurrentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hCurrentStdOut != INVALID_HANDLE_VALUE && hCurrentStdOut != nullptr && GetConsoleWindow() != nullptr) {
            DWORD written = 0;
            WriteConsoleA(hCurrentStdOut, finalMsg, (DWORD)strlen(finalMsg), &written, nullptr);
            g_hConsoleOut = hCurrentStdOut;
        }
    }

    // 2. Output to log file
    if (g_logFile.is_open()) {
        g_logFile << finalMsg;
        g_logFile.flush();
    }

    // 3. Output to debugger
    OutputDebugStringA(finalMsg);
}

} // namespace Logger
