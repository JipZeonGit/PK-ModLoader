#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Mod Metadata
struct PKModInfo {
    const char* name;
    const char* version;
    const char* author;
    const char* description;
};

// Standard Lifecycle Function Signatures
typedef void (*tPK_GetModInfo)(PKModInfo* pOutInfo);
typedef bool (*tPK_ModInit)();
typedef void (*tPK_ModShutdown)();

#define PK_MOD_EXPORT extern "C" __declspec(dllexport)

#ifdef __cplusplus
}
#endif
