#include "ime_fix.h"
#include "../../sdk/logger.h"
#include <MinHook.h>
#include <imm.h>
#include <atomic>
#include <thread>

#pragma comment(lib, "imm32.lib")

namespace ImeFix {

namespace {
    std::atomic<bool> g_bRunning{false};
    std::thread g_hookThread;
    DWORD g_dwHookThreadId = 0;
    HHOOK g_hKeyboardHook = nullptr;
    DWORD g_dwCurrentPid = 0;
    HKL g_hklEnglish = nullptr;
    HWND g_hGameWindow = nullptr;
    WNDPROC g_pOriginalWndProc = nullptr;

    // Function pointer types for Win32 API hooks
    typedef BOOL (WINAPI *PEEKMESSAGEW_T)(LPMSG, HWND, UINT, UINT, UINT);
    typedef BOOL (WINAPI *PEEKMESSAGEA_T)(LPMSG, HWND, UINT, UINT, UINT);
    typedef BOOL (WINAPI *GETMESSAGEW_T)(LPMSG, HWND, UINT, UINT);
    typedef BOOL (WINAPI *GETMESSAGEA_T)(LPMSG, HWND, UINT, UINT);
    typedef SHORT (WINAPI *GETASYNCHRONOUSKEYSTATE_T)(int);
    typedef SHORT (WINAPI *GETKEYSTATE_T)(int);
    typedef UINT (WINAPI *GETRAWINPUTDATA_T)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);

    PEEKMESSAGEW_T fpOriginalPeekMessageW = nullptr;
    PEEKMESSAGEA_T fpOriginalPeekMessageA = nullptr;
    GETMESSAGEW_T fpOriginalGetMessageW = nullptr;
    GETMESSAGEA_T fpOriginalGetMessageA = nullptr;
    GETASYNCHRONOUSKEYSTATE_T fpOriginalGetAsyncKeyState = nullptr;
    GETKEYSTATE_T fpOriginalGetKeyState = nullptr;
    GETRAWINPUTDATA_T fpOriginalGetRawInputData = nullptr;

    inline void ReleaseStuckWinKeys() {
        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
        keybd_event(VK_RWIN, 0, KEYEVENTF_KEYUP, 0);
    }

    // Game Window Procedure subclass - runs directly on the game main thread
    LRESULT CALLBACK SubclassedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
            case WM_INPUTLANGCHANGEREQUEST:
            case WM_INPUTLANGCHANGE: {
                if (g_hklEnglish) {
                    ActivateKeyboardLayout(g_hklEnglish, KLF_ACTIVATE | KLF_REORDER);
                }
                return 0; // Drop input language change completely
            }

            // Swallow all IME messages so no candidate window or IME state change can occur
            case WM_IME_SETCONTEXT:
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_COMPOSITION:
            case WM_IME_ENDCOMPOSITION:
            case WM_IME_NOTIFY:
            case WM_IME_CONTROL:
            case WM_IME_COMPOSITIONFULL:
            case WM_IME_SELECT:
            case WM_IME_CHAR:
            case WM_IME_KEYDOWN:
            case WM_IME_KEYUP: {
                return 0;
            }

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN: {
                // Drop in-game Windows key
                if (wParam == VK_LWIN || wParam == VK_RWIN) {
                    return 0;
                }
                // If any key is pressed and WinKey was logically down in the OS, immediately clear it
                if (fpOriginalGetAsyncKeyState &&
                    ((fpOriginalGetAsyncKeyState(VK_LWIN) & 0x8000) || (fpOriginalGetAsyncKeyState(VK_RWIN) & 0x8000))) {
                    ReleaseStuckWinKeys();
                }
                break;
            }

            case WM_SETFOCUS:
            case WM_ACTIVATE: {
                if (g_hklEnglish) {
                    ActivateKeyboardLayout(g_hklEnglish, KLF_ACTIVATE | KLF_REORDER);
                }
                ImmAssociateContext(hWnd, NULL);
                ReleaseStuckWinKeys();
                break;
            }
        }

        return CallWindowProcW(g_pOriginalWndProc, hWnd, uMsg, wParam, lParam);
    }

    // Process and sanitize any message delivered to the game thread
    void SanitizeGameMessage(LPMSG lpMsg) {
        if (!lpMsg) return;

        // Subclass the main game window on the UI thread when seen
        if (!g_hGameWindow && lpMsg->hwnd) {
            DWORD pid = 0;
            GetWindowThreadProcessId(lpMsg->hwnd, &pid);
            if (pid == g_dwCurrentPid) {
                g_hGameWindow = lpMsg->hwnd;
                ImmAssociateContext(g_hGameWindow, NULL);
                g_pOriginalWndProc = reinterpret_cast<WNDPROC>(
                    SetWindowLongPtrW(g_hGameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubclassedWndProc))
                );
                LOG_INFO("[ImeFix] Attached window subclass to game window (0x%p) on UI thread.", g_hGameWindow);
            }
        }

        // 1. Lock the game thread's keyboard layout to US English (00000409)
        if (g_hklEnglish) {
            HKL curHkl = GetKeyboardLayout(0);
            if (curHkl != g_hklEnglish) {
                ActivateKeyboardLayout(g_hklEnglish, KLF_ACTIVATE | KLF_REORDER);
            }
        }

        // 2. Detach and kill IME context for the window
        if (lpMsg->hwnd) {
            ImmAssociateContext(lpMsg->hwnd, NULL);
        }

        // 3. Filter messages
        switch (lpMsg->message) {
            case WM_INPUTLANGCHANGEREQUEST:
            case WM_INPUTLANGCHANGE: {
                lpMsg->lParam = reinterpret_cast<LPARAM>(g_hklEnglish);
                if (g_hklEnglish) {
                    ActivateKeyboardLayout(g_hklEnglish, KLF_ACTIVATE | KLF_REORDER);
                }
                lpMsg->message = WM_NULL;
                break;
            }

            // Completely swallow all IME events so no candidate window can ever appear
            case WM_IME_SETCONTEXT:
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_COMPOSITION:
            case WM_IME_ENDCOMPOSITION:
            case WM_IME_NOTIFY:
            case WM_IME_CONTROL:
            case WM_IME_COMPOSITIONFULL:
            case WM_IME_SELECT:
            case WM_IME_CHAR:
            case WM_IME_KEYDOWN:
            case WM_IME_KEYUP: {
                lpMsg->message = WM_NULL;
                lpMsg->wParam = 0;
                lpMsg->lParam = 0;
                break;
            }

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN: {
                // Drop in-game Windows key
                if (lpMsg->wParam == VK_LWIN || lpMsg->wParam == VK_RWIN) {
                    lpMsg->message = WM_NULL;
                }
                // If any key is pressed and WinKey is marked down in the OS, force release it!
                if (fpOriginalGetAsyncKeyState &&
                    ((fpOriginalGetAsyncKeyState(VK_LWIN) & 0x8000) || (fpOriginalGetAsyncKeyState(VK_RWIN) & 0x8000))) {
                    ReleaseStuckWinKeys();
                }
                break;
            }

            case WM_SETFOCUS:
            case WM_ACTIVATE: {
                if (g_hklEnglish) {
                    ActivateKeyboardLayout(g_hklEnglish, KLF_ACTIVATE | KLF_REORDER);
                }
                ReleaseStuckWinKeys();
                break;
            }
        }
    }

    // Hooked PeekMessageW
    BOOL WINAPI Hooked_PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
        BOOL res = fpOriginalPeekMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
        if (res && lpMsg) {
            SanitizeGameMessage(lpMsg);
        }
        return res;
    }

    // Hooked PeekMessageA
    BOOL WINAPI Hooked_PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
        BOOL res = fpOriginalPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
        if (res && lpMsg) {
            SanitizeGameMessage(lpMsg);
        }
        return res;
    }

    // Hooked GetMessageW
    BOOL WINAPI Hooked_GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
        BOOL res = fpOriginalGetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
        if (res && lpMsg) {
            SanitizeGameMessage(lpMsg);
        }
        return res;
    }

    // Hooked GetMessageA
    BOOL WINAPI Hooked_GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
        BOOL res = fpOriginalGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
        if (res && lpMsg) {
            SanitizeGameMessage(lpMsg);
        }
        return res;
    }

    // Hooked GetAsyncKeyState - always report Windows key as released (0)
    SHORT WINAPI Hooked_GetAsyncKeyState(int vKey) {
        if (vKey == VK_LWIN || vKey == VK_RWIN) {
            return 0;
        }
        return fpOriginalGetAsyncKeyState(vKey);
    }

    // Hooked GetKeyState - always report Windows key as released (0)
    SHORT WINAPI Hooked_GetKeyState(int vKey) {
        if (vKey == VK_LWIN || vKey == VK_RWIN) {
            return 0;
        }
        return fpOriginalGetKeyState(vKey);
    }

    // Hooked GetRawInputData - filter out Windows key from Keen Engine's Raw Input pipeline
    UINT WINAPI Hooked_GetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) {
        UINT res = fpOriginalGetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
        if (uiCommand == RID_INPUT && pData != nullptr && res != (UINT)-1 && res > 0) {
            RAWINPUT* pRaw = reinterpret_cast<RAWINPUT*>(pData);
            if (pRaw->header.dwType == RIM_TYPEKEYBOARD) {
                if (pRaw->data.keyboard.VKey == VK_LWIN || pRaw->data.keyboard.VKey == VK_RWIN) {
                    pRaw->data.keyboard.VKey = 0;
                    pRaw->data.keyboard.Message = WM_NULL;
                }
            }
        }
        return res;
    }

    // Low-level keyboard hook callback to swallow physical WinKey and prevent Win+W (Widgets)
    LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION) {
            KBDLLHOOKSTRUCT* pKbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

            // Allow our own synthesized KEYUP events through
            if (pKbd->flags & LLKHF_INJECTED) {
                return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
            }

            // Check if foreground window belongs to Portal Knights process
            HWND hFore = GetForegroundWindow();
            DWORD forePid = 0;
            if (hFore) {
                GetWindowThreadProcessId(hFore, &forePid);
            }

            if (forePid == g_dwCurrentPid || hFore == g_hGameWindow) {
                // 1. HARD DROP physical Windows Key completely!
                if (pKbd->vkCode == VK_LWIN || pKbd->vkCode == VK_RWIN) {
                    return 1; // Drop completely!
                }

                // 2. Whenever ANY key is pressed inside the game, ensure WinKey is not stuck down
                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                    if (fpOriginalGetAsyncKeyState &&
                        ((fpOriginalGetAsyncKeyState(VK_LWIN) & 0x8000) || (fpOriginalGetAsyncKeyState(VK_RWIN) & 0x8000))) {
                        ReleaseStuckWinKeys();
                    }
                }
            }
        }
        return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
    }

    // Dedicated hook message pump thread - NO SLEEP, pure Win32 GetMessage pump!
    void HookWorkerLoop() {
        g_dwHookThreadId = GetCurrentThreadId();

        g_hKeyboardHook = SetWindowsHookExW(
            WH_KEYBOARD_LL,
            LowLevelKeyboardProc,
            GetModuleHandleW(nullptr),
            0
        );

        if (!g_hKeyboardHook) {
            LOG_ERROR("[ImeFix] Failed to install WH_KEYBOARD_LL hook! Error: %lu", GetLastError());
            return;
        }

        LOG_INFO("[ImeFix] Low-level keyboard hook installed with zero-latency message pump.");

        // Pure Win32 message pump - blocks cleanly with 0% CPU, handles hooks instantly (<1ms)
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (g_hKeyboardHook) {
            UnhookWindowsHookEx(g_hKeyboardHook);
            g_hKeyboardHook = nullptr;
        }

        LOG_INFO("[ImeFix] Hook worker loop stopped.");
    }
}

bool Initialize() {
    g_dwCurrentPid = GetCurrentProcessId();

    // 1. Disable IME for the entire process
    ImmDisableIME((DWORD)-1);

    // 2. Preload US English keyboard layout
    g_hklEnglish = LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE | KLF_REORDER | KLF_SUBSTITUTE_OK);
    if (!g_hklEnglish) {
        g_hklEnglish = reinterpret_cast<HKL>(static_cast<UINT_PTR>(0x04090409));
    }

    // 3. Initialize MinHook and hook message pump, raw input & key state APIs
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
        LOG_ERROR("[ImeFix] MinHook initialization failed: %s", MH_StatusToString(status));
    } else {
        MH_CreateHookApi(L"user32.dll", "PeekMessageW", reinterpret_cast<LPVOID>(&Hooked_PeekMessageW), reinterpret_cast<LPVOID*>(&fpOriginalPeekMessageW));
        MH_CreateHookApi(L"user32.dll", "PeekMessageA", reinterpret_cast<LPVOID>(&Hooked_PeekMessageA), reinterpret_cast<LPVOID*>(&fpOriginalPeekMessageA));
        MH_CreateHookApi(L"user32.dll", "GetMessageW", reinterpret_cast<LPVOID>(&Hooked_GetMessageW), reinterpret_cast<LPVOID*>(&fpOriginalGetMessageW));
        MH_CreateHookApi(L"user32.dll", "GetMessageA", reinterpret_cast<LPVOID>(&Hooked_GetMessageA), reinterpret_cast<LPVOID*>(&fpOriginalGetMessageA));
        MH_CreateHookApi(L"user32.dll", "GetAsyncKeyState", reinterpret_cast<LPVOID>(&Hooked_GetAsyncKeyState), reinterpret_cast<LPVOID*>(&fpOriginalGetAsyncKeyState));
        MH_CreateHookApi(L"user32.dll", "GetKeyState", reinterpret_cast<LPVOID>(&Hooked_GetKeyState), reinterpret_cast<LPVOID*>(&fpOriginalGetKeyState));
        MH_CreateHookApi(L"user32.dll", "GetRawInputData", reinterpret_cast<LPVOID>(&Hooked_GetRawInputData), reinterpret_cast<LPVOID*>(&fpOriginalGetRawInputData));

        MH_EnableHook(MH_ALL_HOOKS);
        LOG_INFO("[ImeFix] MinHook message, raw input and key-state inline hooks activated.");
    }

    // 4. Start dedicated WH_KEYBOARD_LL hook worker thread
    g_bRunning = true;
    g_hookThread = std::thread(HookWorkerLoop);

    LOG_INFO("[ImeFix] In-game IME & WinKey Blocker successfully initialized.");
    return true;
}

void Shutdown() {
    g_bRunning = false;
    if (g_dwHookThreadId) {
        PostThreadMessageW(g_dwHookThreadId, WM_QUIT, 0, 0);
    }
    if (g_hookThread.joinable()) {
        g_hookThread.join();
    }

    if (g_hGameWindow && g_pOriginalWndProc) {
        SetWindowLongPtrW(g_hGameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_pOriginalWndProc));
        g_pOriginalWndProc = nullptr;
        g_hGameWindow = nullptr;
    }

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    LOG_INFO("[ImeFix] In-game IME & WinKey Blocker cleanly uninstalled.");
}

bool IsActive() {
    return g_bRunning.load();
}

} // namespace ImeFix
