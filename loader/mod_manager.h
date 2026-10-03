#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include "../sdk/PKMod.h"

struct LoadedMod {
    std::wstring fileName;
    HMODULE hModule = nullptr;
    PKModInfo info{};
    tPK_ModShutdown fnShutdown = nullptr;
};

namespace ModManager {
    // Scan mods/ folder and initialize all plugins
    void Initialize();

    // Call PK_ModShutdown on all loaded plugins and unload
    void Shutdown();

    // Get list of active mods
    const std::vector<LoadedMod>& GetLoadedMods();
}
