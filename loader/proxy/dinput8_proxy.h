#pragma once
#include <windows.h>

// ---------------------------------------------------------------------------
// Transparent proxy for the system dinput8.dll.
//
// portal_knights_x64.exe statically imports DINPUT8.dll -> DirectInput8Create
// (verified from the executable's import table), and DINPUT8 is not listed under
// HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\KnownDLLs. Dropping this
// DLL next to the game executable therefore guarantees the loader runs, no
// matter what steam_api64.dll happens to be installed.
//
// The real system DLL is resolved lazily on the first forwarded call, i.e.
// outside the loader lock, and loaded by full path so the loader does not hand
// back our own module (same base name).
// ---------------------------------------------------------------------------
namespace Dinput8Proxy {

    bool Initialize();
    void Uninitialize();
}
