#include "loader_core.h"
#include "logger.h"
#include "mod_manager.h"

#include <imm.h>
#include <atomic>
#include <fstream>
#include <string>

namespace LoaderCore {

namespace {

    // Process-wide owner token. Named mutexes are per-session by default; the
    // "Local\" prefix makes that explicit and keeps us out of other sessions.
    const wchar_t* kOwnerMutexName = L"Local\\PKModLoader.SingleOwner";

    HANDLE g_hOwnerMutex = nullptr;
    std::atomic<bool> g_bOwner{false};

    DWORD WINAPI ModLoaderThread(LPVOID lpParam) {
        (void)lpParam;

        // Sleep briefly to ensure game window, CRT and Steam runtime are loaded
        Sleep(600);

        // 1. Ensure BepInEx-like standard directories exist using absolute paths
        char path[MAX_PATH] = {0};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string s(path);
        size_t pos = s.find_last_of("\\/");
        std::string gameDir = (pos != std::string::npos) ? s.substr(0, pos) : ".";

        std::string modsDir = gameDir + "\\mods";
        std::string pluginsDir = modsDir + "\\plugins";
        std::string configDir = modsDir + "\\config";
        std::string logsDir = modsDir + "\\logs";

        CreateDirectoryA(modsDir.c_str(), nullptr);
        CreateDirectoryA(pluginsDir.c_str(), nullptr);
        CreateDirectoryA(configDir.c_str(), nullptr);
        CreateDirectoryA(logsDir.c_str(), nullptr);

        std::string preferredIni = configDir + "\\ModLoader.ini";
        std::string legacyIni = modsDir + "\\Cores\\ModLoader.ini";
        std::string iniPath = preferredIni;

        // Check if legacy ini exists, otherwise use preferred
        std::ifstream testLegacy(legacyIni);
        if (testLegacy.good()) {
            iniPath = legacyIni;
        }

        // 2. If config does not exist, create default config
        std::ifstream testIni(iniPath);
        if (!testIni.good()) {
            std::ofstream outIni(iniPath);
            if (outIni.is_open()) {
                outIni << "; ==========================================================\n";
                outIni << ";   Portal Knights Native Mod Loader 核心全局配置\n";
                outIni << "; ==========================================================\n\n";
                outIni << "[Logging.Console]\n";
                outIni << "; 是否显示黑框调试控制台窗口 (true 为显示，false 为完全隐藏并在后台静默运行)\n";
                outIni << "Enabled=true\n\n";
                outIni << "; 控制台窗口标题\n";
                outIni << "Title=[Portal Knights] Native Mod Console\n\n";
                outIni << "[Logging.File]\n";
                outIni << "; 是否输出日志文件\n";
                outIni << "Enabled=true\n\n";
                outIni << "; 日志文件保存路径\n";
                outIni << "LogFile=" << logsDir << "\\pk_mod_loader.log\n";
                outIni.close();
            }
        }

        // 3. Read settings from ini
        char consoleEnabledBuf[16] = "true";
        GetPrivateProfileStringA("Logging.Console", "Enabled", "true", consoleEnabledBuf, sizeof(consoleEnabledBuf), iniPath.c_str());
        bool showConsole = (_stricmp(consoleEnabledBuf, "true") == 0 || strcmp(consoleEnabledBuf, "1") == 0);

        char consoleTitleBuf[128] = "[Portal Knights] Native Mod Console";
        GetPrivateProfileStringA("Logging.Console", "Title", "[Portal Knights] Native Mod Console", consoleTitleBuf, sizeof(consoleTitleBuf), iniPath.c_str());

        std::string defaultLogFile = logsDir + "\\pk_mod_loader.log";
        char logFileBuf[MAX_PATH] = {0};
        GetPrivateProfileStringA("Logging.File", "LogFile", defaultLogFile.c_str(), logFileBuf, sizeof(logFileBuf), iniPath.c_str());

        // 4. Initialize Logger
        Logger::Initialize(showConsole, consoleTitleBuf, logFileBuf);

        LOG_INFO("=================================================");
        LOG_INFO("   Portal Knights Native Mod Loader (v1.1.0)     ");
        LOG_INFO("   Architecture: x86-64 Native (Keen Engine)     ");
        LOG_INFO("   GitHub: https://github.com/jipzeongit         ");
        LOG_INFO("=================================================");
        LOG_INFO("[ModLoader] Config loaded from: %s", iniPath.c_str());
        LOG_INFO("[ModLoader] Console Display: %s", showConsole ? "Enabled" : "Disabled (Hidden)");

        // Report the carrier that actually pulled us in -- this is the single
        // most useful line when a mod "does not load" on someone else's machine.
        {
            char exePath[MAX_PATH] = {0};
            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            LOG_INFO("[ModLoader] Host executable: %s", exePath);
            LOG_INFO("[ModLoader] Carrier module: %s",
                     IsOwner() ? "dinput8.dll proxy" : "secondary proxy (not the owner)");
        }

        // Scan and load all mod plugins from mods/ directory
        ModManager::Initialize();

        return 0;
    }

    // Keyboard / IME hardening. Running it from the DllMain of an early-loaded
    // carrier means it happens before any game window exists.
    void ApplyInputHardening() {
        ImmDisableIME((DWORD)-1);
        LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE | KLF_REORDER);
        ActivateKeyboardLayout((HKL)0x04090409, KLF_ACTIVATE | KLF_REORDER);
    }

} // namespace

bool IsOwner() {
    return g_bOwner.load();
}

void OnProcessAttach(HMODULE hSelf) {
    (void)hSelf;

    // Single-owner gate: only the first proxy module to get here may start the
    // loader thread. Without it, a second installed copy of the proxy would
    // inject every mod twice (double hooks, double hotkey listeners).
    SetLastError(ERROR_SUCCESS);
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, kOwnerMutexName);
    DWORD createErr = GetLastError();

    if (hMutex != nullptr && createErr == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        OutputDebugStringA("[PK Loader] Another proxy DLL already started the loader; skipping.\n");
        return;
    }

    // Fail open: if the mutex could not be created we still want the mods to
    // load rather than silently doing nothing.
    g_hOwnerMutex = hMutex;
    g_bOwner = true;

    ApplyInputHardening();

    // Spawn loader thread asynchronously to avoid loader lock
    HANDLE hThread = CreateThread(nullptr, 0, ModLoaderThread, nullptr, 0, nullptr);
    if (hThread) {
        CloseHandle(hThread);
    }
}

void OnProcessDetach() {
    if (!g_bOwner.exchange(false)) {
        return;
    }

    ModManager::Shutdown();
    Logger::Shutdown();

    if (g_hOwnerMutex) {
        CloseHandle(g_hOwnerMutex);
        g_hOwnerMutex = nullptr;
    }
}

} // namespace LoaderCore
