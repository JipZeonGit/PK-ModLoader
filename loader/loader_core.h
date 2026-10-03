#pragma once
#include <windows.h>

// ---------------------------------------------------------------------------
// Bootstrap for the PK-ModLoader proxy DLL.
//
// The loader is a Windows proxy DLL, so it only ever runs if the game actually
// loads it. Choosing the carrier therefore matters: it must be a DLL that
// portal_knights_x64.exe imports directly, and it must NOT be a KnownDLL
// (KnownDLLs are always resolved to System32 and can never be overridden by a
// copy sitting next to the executable).
//
// `dinput8.dll` satisfies both: the game executable imports
// `DINPUT8.dll -> DirectInput8Create` statically, and the system DLL exposes
// only 6 exports, so a complete and ordinal-correct forward is trivial.
//
// Relying on a DLL that the executable does NOT import is what silently breaks
// on machines where the third-party module that used to pull it in is missing:
// the loader never starts, no console appears and no log file is ever written.
//
// The named mutex in OnProcessAttach keeps the mods loaded exactly once even if
// more than one copy of the proxy ends up installed.
// ---------------------------------------------------------------------------
namespace LoaderCore {

    // Call from DllMain(DLL_PROCESS_ATTACH). The first proxy to arrive wins and
    // starts the loader thread; any later one becomes a no-op.
    void OnProcessAttach(HMODULE hSelf);

    // Call from DllMain(DLL_PROCESS_DETACH) only when the process is not
    // terminating (lpReserved == nullptr).
    void OnProcessDetach();

    // True when this module is the one that owns the loader.
    bool IsOwner();
}
