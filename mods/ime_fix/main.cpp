#include <windows.h>
#include "../../sdk/PKMod.h"
#include "ime_fix.h"

PK_MOD_EXPORT void PK_GetModInfo(PKModInfo* pOutInfo) {
    if (!pOutInfo) return;
    pOutInfo->name = "Win10/11 IME & Widgets Suppressor";
    pOutInfo->version = "1.1.0";
    pOutInfo->author = "jipzeongit";
    pOutInfo->description = "Shields WinKey, fixes Win+W widgets / magnifier popup, and provides dual-mode Chinese IME.";
}

PK_MOD_EXPORT bool PK_ModInit() {
    return ImeFix::Initialize();
}

PK_MOD_EXPORT void PK_ModShutdown() {
    ImeFix::Shutdown();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
