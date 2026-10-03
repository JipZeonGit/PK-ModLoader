<div align="center">

# Portal Knights Native Mod Framework (PK-ModLoader)

</div>

<div align="center">

A BepInEx-style Native C++ Mod Loader and Plugin Framework for Portal Knights (x86-64)  
《传送门骑士》（x86-64 原生 C++）的 BepInEx 式解耦原生 Mod 加载器与插件扩展框架

</div>

<p align="center">
  <img src="https://img.shields.io/badge/Game-Portal_Knights-FF7700?style=flat&logo=steam&logoColor=white" alt="Game Portal Knights">
  <img src="https://img.shields.io/badge/Arch-x86--64-0078D7?style=flat&logo=windows&logoColor=white" alt="Architecture x64">
  <img src="https://img.shields.io/badge/Engine-Keen_Engine-555555?style=flat" alt="Keen Engine">
  <img src="https://img.shields.io/badge/Loader-dinput8.dll-4CAF50?style=flat" alt="Loader DLL">
  <br>
  <img src="https://img.shields.io/badge/C%2B%2B-Standard_20-00599C?style=flat&logo=c%2B%2B&logoColor=white" alt="C++20">
  <img src="https://img.shields.io/badge/Hook-MinHook_v1.3.3-8A2BE2?style=flat" alt="MinHook v1.3.3">
  <img src="https://img.shields.io/badge/Architecture-BepInEx_Style-blueviolet?style=flat" alt="BepInEx Style Architecture">
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=flat" alt="License MIT">
</p>

<div align="center">

[English](#english) | [简体中文](#简体中文)

</div>

---

<a name="english"></a>
# English

## Overview

**PK-ModLoader** is a high-performance, non-intrusive native C++ modding framework built specifically for *Portal Knights* (Keen Engine, 64-bit DirectX 11).

Adopting the proven modular design of **BepInEx**, the project cleanly separates the core runtime loader from game plugins. Mod DLLs, configurations, and logs are segregated into distinct subdirectories, giving players and developers a tidy, modular, and maintainable modding environment.

The repository ships the loader plus one built-in plugin: **PK_ImeFix**, which fixes the long-standing Chinese IME and sticky-WinKey problems on Chinese Windows 10/11.

---

## Directory Architecture

The framework organizes files into a clean **BepInEx-style** structure:

```text
Portal Knights/
├── portal_knights_x64.exe
├── dinput8.dll                     <-- [Core Loader] transparent proxy DLL
└── mods/
    ├── plugins/                    <-- [Mod Plugins] Place all your mod .dll files here
    │   └── PK_ImeFix.dll           <-- Built-in: Chinese IME lock & WinKey popup suppressor
    ├── config/                     <-- [Configurations] Auto-generated INI files for all mods
    │   └── ModLoader.ini           <-- Global loader settings (console toggle, logging)
    └── logs/                       <-- [Logs] Runtime logs output folder
        └── pk_mod_loader.log
```

---

## How the Loader Gets Loaded

The loader is a transparent proxy DLL, and it can only do its job if the game actually loads it. The DLL name it is shipped under is therefore not cosmetic — it has to satisfy two conditions:

1. **`portal_knights_x64.exe` must import it directly.** A proxy only starts if the executable (or one of its imports) asks for that exact module name at startup.
2. **It must not be a KnownDLL.** Windows always resolves KnownDLLs to `System32`, so a same-named copy sitting next to the executable could never take effect.

The loader is shipped as **`dinput8.dll`**, which satisfies both:

* the game executable statically imports `DINPUT8.dll → DirectInput8Create`;
* `DINPUT8` is not a KnownDLL;
* the system DLL exposes only 6 exports, so a complete, ordinal-correct forward is trivial and DirectInput keeps working normally.

Relying on a DLL name that the executable does **not** import is the classic way to end up with a mod loader that works on one machine and silently does nothing on another: it only loads when some unrelated third-party module happens to pull that name in, and it fails with no error at all.

> **Troubleshooting "mods don't load":** check whether `mods/logs/pk_mod_loader.log` exists.
> If it does **not** exist, the loader was never loaded into the process — verify that `dinput8.dll`
> sits next to `portal_knights_x64.exe`. When it does load, the log line
> `[ModLoader] Carrier module:` confirms which module started the loader.

---

## Built-in Plugin: `PK_ImeFix.dll` — Chinese IME & Sticky WinKey Suppressor

Resolves the persistent issue where typing keys like `W`, `A`, `S`, `D` triggers Chinese IME candidates, causes input lag, or accidentally opens Windows shortcuts (e.g., `Win+W` Widgets on Windows 11, Windows Ink Workspace, `Win+I` Settings, or `Win+U` Magnifier).

* **100% In-Process Game Memory Hook**: Operates strictly within the game process via MinHook. Does **not** modify your host Windows operating system, registry, or system settings.
* **Engine Message Pump Interception**: Intercepts `PeekMessageW/A` and `GetMessageW/A`, locks the active keyboard layout to US English (`00000409`), detaches IME input contexts via `ImmAssociateContext(NULL)`, and drops IME composition messages.
* **Raw Input Shielding**: Filters out `VK_LWIN` / `VK_RWIN` scan codes from Keen Engine's Raw Input stream, preventing stuck modifier keys and unexpected system popups.
* **Low-Level Keyboard Hook**: Installs a zero-latency keyboard hook on the game window so the shielding keeps working even when the engine's own message pump is bypassed.

---

## Configuration Guide

Configuration files are located in `mods/config/` (auto-generated on the first launch):

### `mods/config/ModLoader.ini`

```ini
[Logging.Console]
; Show black debug console window (set to false to run completely silent in background)
Enabled=true

; Console window title
Title=[Portal Knights] Native Mod Console

[Logging.File]
; Output runtime log file
Enabled=true
LogFile=mods/logs/pk_mod_loader.log
```

---

## Installation

1. Download the latest release package or build from source.
2. Copy the contents of the `output/` folder directly into your *Portal Knights* game root directory (where `portal_knights_x64.exe` is located):
   * `dinput8.dll` → Game root directory.
   * `mods/` → Game root directory.
3. Launch the game normally via Steam or executable.

---

## Building from Source

### Prerequisites
* Linux / WSL (Ubuntu or Debian recommended)
* `x86_64-w64-mingw32-g++` (MinGW-w64 cross compiler)
* `make`

### Build Command
```bash
# In WSL or Linux terminal:
make
```

All compiled binaries will be automatically placed in the `output/` directory:
* `output/dinput8.dll` — the core loader
* `output/mods/plugins/PK_ImeFix.dll`

The build is fully static: the produced DLLs import only `msvcrt` / `kernel32` / `user32` / `imm32`, so **no Visual C++ Redistributable is required** on the target machine.

> **No game files are needed to build.** See [`docs/GAME-FILES.md`](docs/GAME-FILES.md) for the complete list of build requirements, the runtime requirements, and which game files must never be committed to this repository.

---

## Mod Developer SDK (`sdk/PKMod.h`)

Developing a new native mod plugin takes just a few lines of C++:

```cpp
#include "../../sdk/PKMod.h"

// 1. Export Mod Metadata
PK_MOD_EXPORT void PK_GetModInfo(PKModInfo* info) {
    info->name        = "Custom Mod";
    info->version     = "1.0.0";
    info->author      = "YourName";
    info->description = "Does something awesome in Portal Knights.";
}

// 2. Mod Initialization (called automatically on game startup)
PK_MOD_EXPORT bool PK_ModInit() {
    // Initialize your MinHook hooks, memory patches, or worker threads
    return true;
}

// 3. Mod Shutdown (called automatically on game exit)
PK_MOD_EXPORT void PK_ModShutdown() {
    // Remove hooks and free resources
}
```

*Note: The loader also supports standard DLLs without these export signatures via `LoadLibraryW`.*

---

<br>

<a name="简体中文"></a>
# 简体中文

## 项目概述

**PK-ModLoader** 是专为《传送门骑士》（*Portal Knights*，Keen Engine 64位 DirectX 11 引擎）打造的高性能、无侵入式原生 C++ Mod 加载框架。

项目深度借鉴了主流游戏模组框架 **BepInEx** 的解耦设计哲学，将核心加载器与具体的 Mod 插件彻底分离。插件、配置文件、运行日志拥有独立归类的子目录，让玩家和开发者拥有极其清爽、规范、易维护的 Mod 运行环境。

仓库包含加载器本体，以及一个内置插件 **PK_ImeFix**，用于修复中文 Windows 10/11 下长期存在的输入法与 Win 键误触问题。

---

## 目录架构

框架采用规范的 **BepInEx 式三层分类目录**：

```text
Portal Knights 游戏根目录/
├── portal_knights_x64.exe
├── dinput8.dll                     <-- 【核心加载器】透明代理 DLL
└── mods/
    ├── plugins/                    <-- 【插件目录】所有 Mod 的 .dll 文件均存放于此
    │   └── PK_ImeFix.dll           <-- 内置插件：中文输入法屏蔽与 Win 快捷键防误触补丁
    ├── config/                     <-- 【配置目录】各插件自动生成的 INI 配置文件
    │   └── ModLoader.ini           <-- 加载器全局配置（黑框控制台开关、日志输出设置）
    └── logs/                       <-- 【日志目录】日志文件存放路径
        └── pk_mod_loader.log
```

---

## 加载器是如何被加载的

加载器是一个透明的代理 DLL，只有**被游戏真正加载**才能工作。因此它对外使用的 DLL 名字并不是随便取的，必须同时满足两个条件：

1. **`portal_knights_x64.exe` 必须直接导入它。** 代理 DLL 只有在主程序（或其某个导入项）启动时请求了这个确切的模块名，才会被加载。
2. **它不能是 KnownDLL。** Windows 对 KnownDLL 永远从 `System32` 解析，放在主程序同级的同名副本根本不可能生效。

本加载器以 **`dinput8.dll`** 的形式发布，正好满足这两点：

* 游戏主程序**静态导入**了 `DINPUT8.dll → DirectInput8Create`；
* `DINPUT8` **不在** KnownDLLs 列表中；
* 系统 `dinput8.dll` 只有 **6 个导出**，转发实现简单、能完整保留导出序号，DirectInput 功能不受影响。

反过来说，**依赖一个主程序并不导入的 DLL 名字，正是「在别人机器上静默失效」的经典原因**：只有当某个不相关的第三方模块恰好把那个名字拉进进程时它才会被加载，一旦那个模块不存在，加载器就完全不启动，而且**一个错误都不会报**。

> **排查「mod 不生效」：** 先看游戏根目录有没有 `mods/logs/pk_mod_loader.log`。
> **如果这个文件压根不存在**，说明加载器从未被加载 —— 确认 `dinput8.dll` 与 `portal_knights_x64.exe` 在同一目录。
> 加载成功时，日志里的 `[ModLoader] Carrier module:` 一行会告诉你实际生效的载体模块。

---

## 内置精选插件：`PK_ImeFix.dll` — 中文输入法锁定与 Win 键防误触修复

彻底根治《传送门骑士》在中文 Windows 10/11 系统下按 `W`、`A`、`S`、`D` 误触发微软输入法候选框、按键卡死，或因按键粘滞意外触发 Windows 系统快捷键（如 Win11 的 `Win+W` 小组件、Windows Ink 工作区、`Win+I` 系统设置、`Win+U` 放大镜等）的恶性问题。

* **100% 游戏进程内钩子，零系统侵入**：全程通过 MinHook 在游戏内存中拦截，**绝对不修改**宿主系统的注册表、系统设置或语言包。
* **引擎主消息泵拦截**：拦截 `PeekMessageW/A` 和 `GetMessageW/A`，将当前线程键盘布局强制锁定为美式英文（`00000409`），解除窗口 IME 关联上下文（`ImmAssociateContext(NULL)`），阻断一切拼音候选框消息。
* **原始输入底层护盾**：在 Keen 引擎的 Raw Input 消息流中剔除 `VK_LWIN` / `VK_RWIN` 扫描码，防止按键状态粘连导致的快捷键误触发。
* **低级键盘钩子**：在游戏窗口上安装零延迟键盘钩子，即使引擎自身的消息泵被绕过，屏蔽依然有效。

---

## 配置文件指南

配置文件存放在 `mods/config/` 目录中（首次运行游戏时会自动生成）：

### `mods/config/ModLoader.ini`（核心加载器配置）

```ini
[Logging.Console]
; 是否显示黑框调试控制台窗口 (设为 false 则完全静默后台运行，不弹出任何黑框)
Enabled=true

; 控制台窗口标题
Title=[Portal Knights] Native Mod Console

[Logging.File]
; 是否输出日志文件
Enabled=true
LogFile=mods/logs/pk_mod_loader.log
```

---

## 安装与使用

1. 下载最新 Release 发布包，或自行编译源码。
2. 将 `output/` 文件夹内的文件**直接复制**到《传送门骑士》游戏根目录（即与 `portal_knights_x64.exe` 同级的目录）：
   * `output/dinput8.dll` → 游戏根目录。
   * `output/mods/` 文件夹 → 游戏根目录。
3. 像往常一样从 Steam 启动游戏即可生效！

---

## 源码编译

### 环境需求
* Linux 或 Windows WSL（推荐 Ubuntu / Debian）
* `x86_64-w64-mingw32-g++`（MinGW-w64 交叉编译器）
* `make`

### 编译命令
```bash
# 在 WSL 或 Linux 终端中运行：
make
```

编译完成后，全部打包产物会自动输出至 `output/` 目录：
* `output/dinput8.dll` —— 核心加载器
* `output/mods/plugins/PK_ImeFix.dll`

编译为完全静态链接：产物只依赖 `msvcrt` / `kernel32` / `user32` / `imm32`，因此目标机器**无需安装任何 Visual C++ 运行库**。

> **编译本项目不需要任何游戏文件。** 完整的编译依赖、运行期要求，以及哪些游戏文件绝对不能提交到本仓库，见 [`docs/GAME-FILES.md`](docs/GAME-FILES.md)。

---

## Mod 插件开发 SDK (`sdk/PKMod.h`)

如果想开发自己的 Mod 插件，只需引入 SDK 并实现 3 个简单的 C 导出函数：

```cpp
#include "../../sdk/PKMod.h"

// 1. 返回 Mod 元数据信息
PK_MOD_EXPORT void PK_GetModInfo(PKModInfo* info) {
    info->name        = "我的自定义 Mod";
    info->version     = "1.0.0";
    info->author      = "YourName";
    info->description = "为传送门骑士添加新特性的 Mod。";
}

// 2. Mod 加载初始化（在游戏启动时由 Loader 自动调用）
PK_MOD_EXPORT bool PK_ModInit() {
    // 编写你的 MinHook 钩子、内存补丁或后台线程逻辑
    return true;
}

// 3. Mod 卸载清理（在游戏退出时由 Loader 自动调用）
PK_MOD_EXPORT void PK_ModShutdown() {
    // 卸载钩子并释放内存
}
```

*提示：加载器向下兼容所有未导出此规范接口的标准 Windows DLL，会通过 `LoadLibraryW` 自动加载运行。*

---

## 开源协议

本项目采用 [MIT 许可证](LICENSE) 开源。
