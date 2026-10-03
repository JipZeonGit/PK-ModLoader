#pragma once
#include <windows.h>

namespace ImeFix {
    // Starts the dedicated background hook thread and completely suppresses IME and WinKey
    bool Initialize();

    // Shuts down the background thread and releases hooks
    void Shutdown();

    // Check if the blocker is running
    bool IsActive();
}
