#include <windows.h>
#include "loader_core.h"
#include "proxy/dinput8_proxy.h"

// ---------------------------------------------------------------------------
// Carrier #2: dinput8.dll
//
// This is the RECOMMENDED carrier. portal_knights_x64.exe imports DINPUT8.dll
// -> DirectInput8Create directly, and DINPUT8 is not a KnownDLL, so the copy
// sitting in the game folder is always the one that gets loaded.
//
// The real system dinput8.dll is deliberately NOT loaded here: doing it from
// DllMain would run LoadLibrary under the loader lock. It is resolved lazily by
// the first forwarded call instead.
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(hModule);
            LoaderCore::OnProcessAttach(hModule);
            break;
        }

        case DLL_PROCESS_DETACH: {
            if (!lpReserved) {
                LoaderCore::OnProcessDetach();
                Dinput8Proxy::Uninitialize();
            }
            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}
