CXX = x86_64-w64-mingw32-g++
CC  = x86_64-w64-mingw32-gcc

CXXFLAGS = -O2 -std=c++17 -Wall -Wno-unused-function
LDFLAGS  = -shared -static -s

# Output directories (BepInEx-like standard layout)
OUT_DIR     = output
MODS_OUT    = $(OUT_DIR)/mods
PLUGINS_OUT = $(MODS_OUT)/plugins
CONFIG_OUT  = $(MODS_OUT)/config
LOGS_OUT    = $(MODS_OUT)/logs

# MinHook source files
MINHOOK_SRC = sdk/minhook/src/buffer.c \
              sdk/minhook/src/hook.c \
              sdk/minhook/src/trampoline.c \
              sdk/minhook/src/hde/hde64.c

# Shared SDK includes
SDK_INC     = -Isdk -Isdk/minhook/include -Isdk/minhook/src

# Target binaries
LOADER_DLL  = $(OUT_DIR)/dinput8.dll
IME_MOD_DLL = $(PLUGINS_OUT)/PK_ImeFix.dll

# `dirs` is an ORDER-ONLY prerequisite of every target that writes into output/.
# Listing it as a normal prerequisite (or as a prerequisite of `all`) lets
# `make -j` start a compile before `mkdir` has finished, which fails the build.
.PHONY: all dirs clean

all: $(LOADER_DLL) $(IME_MOD_DLL)
	@echo "=========================================================="
	@echo "Build successful! BepInEx-style structure created in $(OUT_DIR)/"
	@echo "  1. $(LOADER_DLL)        [Core Mod Loader]"
	@echo "  2. $(IME_MOD_DLL)   [Plugin: Chinese IME / WinKey fix]"
	@echo "=========================================================="

dirs:
	@mkdir -p $(PLUGINS_OUT) $(CONFIG_OUT) $(LOGS_OUT)

# 1. Core Loader, shipped as dinput8.dll.
#    portal_knights_x64.exe statically imports DINPUT8.dll -> DirectInput8Create,
#    and DINPUT8 is not a KnownDLL, so this copy always wins.
$(LOADER_DLL): loader/dinput8main.cpp loader/loader_core.cpp loader/loader_core.h \
               loader/mod_manager.cpp loader/logger.cpp \
               loader/proxy/dinput8_proxy.cpp loader/proxy/dinput8_proxy.h loader/proxy/dinput8.def | dirs
	$(CXX) $(CXXFLAGS) -Iloader $(SDK_INC) -o $@ \
		loader/dinput8main.cpp \
		loader/loader_core.cpp \
		loader/mod_manager.cpp \
		loader/logger.cpp \
		loader/proxy/dinput8_proxy.cpp \
		$(LDFLAGS) loader/proxy/dinput8.def -limm32

# 2. Build Plugin: PK_ImeFix.dll -- Chinese IME & sticky WinKey suppressor
$(IME_MOD_DLL): mods/ime_fix/main.cpp mods/ime_fix/ime_fix.cpp sdk/logger.cpp $(MINHOOK_SRC) | dirs
	$(CXX) $(CXXFLAGS) -Imods/ime_fix $(SDK_INC) -o $@ \
		mods/ime_fix/main.cpp \
		mods/ime_fix/ime_fix.cpp \
		sdk/logger.cpp \
		$(MINHOOK_SRC) \
		$(LDFLAGS) -limm32

clean:
	rm -rf $(OUT_DIR)
