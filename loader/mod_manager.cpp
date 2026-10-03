#include "mod_manager.h"
#include "logger.h"
#include <filesystem>

namespace ModManager {

namespace {
    std::vector<LoadedMod> g_loadedMods;

    std::wstring GetGameDirectory() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring ws(path);
        size_t pos = ws.find_last_of(L"\\/");
        return (pos != std::wstring::npos) ? ws.substr(0, pos) : L".";
    }
}

void Initialize() {
    std::wstring gameDir = GetGameDirectory();
    std::wstring modsDir = gameDir + L"\\mods";
    std::wstring pluginsDir = modsDir + L"\\plugins";
    std::wstring configDir = modsDir + L"\\config";
    std::wstring logsDir = modsDir + L"\\logs";

    // Auto-create standard BepInEx-like mod directory hierarchy
    CreateDirectoryW(modsDir.c_str(), nullptr);
    CreateDirectoryW(pluginsDir.c_str(), nullptr);
    CreateDirectoryW(configDir.c_str(), nullptr);
    CreateDirectoryW(logsDir.c_str(), nullptr);

    LOG_INFO("[ModLoader] Game Directory   : %ls", gameDir.c_str());
    LOG_INFO("[ModLoader] Mods Root        : %ls", modsDir.c_str());
    LOG_INFO("[ModLoader] Plugins Directory: %ls", pluginsDir.c_str());
    LOG_INFO("[ModLoader] Config Directory : %ls", configDir.c_str());
    LOG_INFO("[ModLoader] Logs Directory   : %ls", logsDir.c_str());

    std::vector<std::pair<std::wstring, std::wstring>> dllList; // <fileName, fullPath>

    // 1. Primary search: mods/plugins/*.dll
    auto ScanFolder = [&](const std::wstring& folderPath) {
        std::wstring searchPattern = folderPath + L"\\*.dll";
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring name = findData.cFileName;
                    // Check if already in list
                    bool exists = false;
                    for (const auto& item : dllList) {
                        if (_wcsicmp(item.first.c_str(), name.c_str()) == 0) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        dllList.push_back({name, folderPath + L"\\" + name});
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    };

    ScanFolder(pluginsDir);
    ScanFolder(modsDir); // Fallback: also load DLLs placed directly in mods/

    if (dllList.empty()) {
        LOG_INFO("[ModLoader] No .dll mods found in %ls. Waiting for mods...", pluginsDir.c_str());
        return;
    }

    LOG_INFO("[ModLoader] Discovered %zu mod plugin(s).", dllList.size());

    for (size_t i = 0; i < dllList.size(); ++i) {
        const auto& fileName = dllList[i].first;
        const auto& fullPath = dllList[i].second;

        LOG_INFO("--------------------------------------------------");
        LOG_INFO("[ModLoader] [%zu/%zu] Loading: %ls", i + 1, dllList.size(), fileName.c_str());

        HMODULE hMod = LoadLibraryW(fullPath.c_str());
        if (!hMod) {
            LOG_ERROR("[ModLoader] Failed to load DLL! Error code: %lu", GetLastError());
            continue;
        }

        LoadedMod modRecord;
        modRecord.fileName = fileName;
        modRecord.hModule = hMod;

        auto fnGetInfo = reinterpret_cast<tPK_GetModInfo>(GetProcAddress(hMod, "PK_GetModInfo"));
        auto fnInit = reinterpret_cast<tPK_ModInit>(GetProcAddress(hMod, "PK_ModInit"));
        modRecord.fnShutdown = reinterpret_cast<tPK_ModShutdown>(GetProcAddress(hMod, "PK_ModShutdown"));

        if (fnGetInfo) {
            fnGetInfo(&modRecord.info);
            LOG_INFO("[ModLoader]   Name       : %s", modRecord.info.name ? modRecord.info.name : "Unknown");
            LOG_INFO("[ModLoader]   Version    : %s", modRecord.info.version ? modRecord.info.version : "0.0.0");
            LOG_INFO("[ModLoader]   Author     : %s", modRecord.info.author ? modRecord.info.author : "Unknown");
            if (modRecord.info.description) {
                LOG_INFO("[ModLoader]   Description: %s", modRecord.info.description);
            }
        } else {
            LOG_INFO("[ModLoader]   (Legacy / Injected DLL without PK_GetModInfo)");
        }

        bool initSuccess = true;
        if (fnInit) {
            initSuccess = fnInit();
            if (initSuccess) {
                LOG_INFO("[ModLoader]   Status     : Successfully Initialized [OK]");
            } else {
                LOG_WARN("[ModLoader]   Status     : Initialization returned FALSE [FAILED]");
            }
        } else {
            LOG_INFO("[ModLoader]   Status     : Loaded into memory [OK]");
        }

        if (initSuccess) {
            g_loadedMods.push_back(modRecord);
        }
    }

    LOG_INFO("==================================================");
    LOG_INFO("[ModLoader] Total active mods running: %zu", g_loadedMods.size());
    LOG_INFO("==================================================");
}

void Shutdown() {
    LOG_INFO("[ModLoader] Shutting down active mods in reverse order...");
    for (auto it = g_loadedMods.rbegin(); it != g_loadedMods.rend(); ++it) {
        if (it->fnShutdown) {
            LOG_INFO("[ModLoader] Shutting down: %ls", it->fileName.c_str());
            it->fnShutdown();
        }
        if (it->hModule) {
            FreeLibrary(it->hModule);
        }
    }
    g_loadedMods.clear();
}

const std::vector<LoadedMod>& GetLoadedMods() {
    return g_loadedMods;
}

} // namespace ModManager
