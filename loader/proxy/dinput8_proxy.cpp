#include "dinput8_proxy.h"

namespace {
    HMODULE g_hOriginalDll = nullptr;

    FARPROC pDirectInput8Create = nullptr;
    FARPROC pDllCanUnloadNow = nullptr;
    FARPROC pDllGetClassObject = nullptr;
    FARPROC pDllRegisterServer = nullptr;
    FARPROC pDllUnregisterServer = nullptr;
    FARPROC pGetdfDIJoystick = nullptr;
}

namespace Dinput8Proxy {

bool Initialize() {
    if (g_hOriginalDll) {
        return true;
    }

    wchar_t systemPath[MAX_PATH];
    if (GetSystemDirectoryW(systemPath, MAX_PATH) == 0) {
        return false;
    }

    wcscat_s(systemPath, L"\\dinput8.dll");
    g_hOriginalDll = LoadLibraryW(systemPath);
    if (!g_hOriginalDll) {
        return false;
    }

#define LOAD_FUNC(name) p##name = GetProcAddress(g_hOriginalDll, #name)
    LOAD_FUNC(DirectInput8Create);
    LOAD_FUNC(DllCanUnloadNow);
    LOAD_FUNC(DllGetClassObject);
    LOAD_FUNC(DllRegisterServer);
    LOAD_FUNC(DllUnregisterServer);
    LOAD_FUNC(GetdfDIJoystick);
#undef LOAD_FUNC

    return true;
}

void Uninitialize() {
    if (g_hOriginalDll) {
        FreeLibrary(g_hOriginalDll);
        g_hOriginalDll = nullptr;
    }
}

} // namespace Dinput8Proxy

// Forwarded exports. The system dinput8.dll exports exactly these six symbols,
// with DirectInput8Create at ordinal 1 (see loader/proxy/dinput8.def).
extern "C" {

HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, const void* riidltf, void** ppvOut, void* punkOuter) {
    if (!pDirectInput8Create) Dinput8Proxy::Initialize();
    typedef HRESULT(WINAPI* Func)(HINSTANCE, DWORD, const void*, void**, void*);
    if (!pDirectInput8Create) return (HRESULT)0x80004005L; // E_FAIL
    return ((Func)pDirectInput8Create)(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

HRESULT WINAPI DllCanUnloadNow(void) {
    if (!pDllCanUnloadNow) Dinput8Proxy::Initialize();
    typedef HRESULT(WINAPI* Func)(void);
    if (!pDllCanUnloadNow) return (HRESULT)0x80004005L; // E_FAIL
    return ((Func)pDllCanUnloadNow)();
}

// NOTE: the signature must match the one combaseapi.h already declares
// (REFCLSID / REFIID are C++ references, still pointer-sized on x64).
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    if (!pDllGetClassObject) Dinput8Proxy::Initialize();
    typedef HRESULT(WINAPI* Func)(REFCLSID, REFIID, LPVOID*);
    if (!pDllGetClassObject) return (HRESULT)0x80004005L; // E_FAIL
    return ((Func)pDllGetClassObject)(rclsid, riid, ppv);
}

HRESULT WINAPI DllRegisterServer(void) {
    if (!pDllRegisterServer) Dinput8Proxy::Initialize();
    typedef HRESULT(WINAPI* Func)(void);
    if (!pDllRegisterServer) return (HRESULT)0x80004005L; // E_FAIL
    return ((Func)pDllRegisterServer)();
}

HRESULT WINAPI DllUnregisterServer(void) {
    if (!pDllUnregisterServer) Dinput8Proxy::Initialize();
    typedef HRESULT(WINAPI* Func)(void);
    if (!pDllUnregisterServer) return (HRESULT)0x80004005L; // E_FAIL
    return ((Func)pDllUnregisterServer)();
}

void* WINAPI GetdfDIJoystick(void) {
    if (!pGetdfDIJoystick) Dinput8Proxy::Initialize();
    typedef void*(WINAPI* Func)(void);
    if (!pGetdfDIJoystick) return nullptr;
    return ((Func)pGetdfDIJoystick)();
}

} // extern "C"
