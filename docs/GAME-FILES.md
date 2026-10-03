# Game File Dependencies

> **TL;DR — building this project requires _no_ game files at all.**
> Portal Knights ships no headers, no import libraries and no SDK, and none of the
> code in this repository references a game symbol at link time. `make` builds
> everything from the sources in this repository using MinGW-w64 alone.

---

## English

### 1. What the build actually needs

| Requirement | Why |
| --- | --- |
| `x86_64-w64-mingw32-g++` / `-gcc` | MinGW-w64 cross compiler (Linux/WSL) |
| `make` | drives the build |
| This repository's own sources | `loader/`, `mods/ime_fix/`, `sdk/` |

That is the complete list. **Nothing from the game is required, and nothing from the game is linked against.**

### 2. Why no game files are needed

* **The loader** (`dinput8.dll`) is a pure *forwarder*: it resolves the real system
  `C:\Windows\System32\dinput8.dll` at runtime by full path and forwards its six
  exports. It never imports a game symbol.
* **`PK_ImeFix`** hooks **Windows API functions**, not game functions — for example
  `PeekMessageW/A`, `GetMessageW/A`, `GetAsyncKeyState`, `ImmAssociateContext`, and
  the game window's message loop. It contains **no hardcoded game RVA / offset**, so
  it does not need the executable at build time (and it is not tied to one game
  build at runtime either).
* The loader's mod scanning (`loader/mod_manager.cpp`) is resolved at runtime from
  the directory of the host executable — again, no build-time dependency.

### 3. What is needed at *runtime* (not at build time)

Copied into the game root, next to `portal_knights_x64.exe`:

| File | Requirement |
| --- | --- |
| `dinput8.dll` | must sit next to the game executable; see below |
| `mods/plugins/*.dll` | loaded by the loader from the game directory |

The loader only starts if the game process actually loads it. Because the DLL is
named `dinput8.dll`, that requires the game executable to import
`DINPUT8.dll → DirectInput8Create`, and requires `DINPUT8` **not** to be a KnownDLL.
`portal_knights_x64.exe` satisfies both — that is exactly why the loader is shipped
under this name rather than some other DLL name.

### 4. Optional: verifying that premise yourself

If you want to verify the loader's premise on **your own** copy of the game, you
only need `portal_knights_x64.exe` locally, and you can inspect its import table:

```bash
# Linux / WSL, with MinGW-w64 or binutils installed
x86_64-w64-mingw32-objdump -p "portal_knights_x64.exe" | grep "DLL Name"
```

You should see `DINPUT8.dll` in that list. (It also confirms which other system
modules the game imports.)

This is entirely optional and purely for your own inspection.

### 5. Local research copies must never be committed

While reverse-engineering it is normal to keep a local copy of the game next to the
project. **Portal Knights is commercial, copyrighted software and none of it may be
committed to this repository.**

The root `.gitignore` enforces this. It excludes, among others:

* `Reference/` — a full local copy of the game (can be many GB)
* `extracted_reference/` — game files unpacked for inspection
* `*.kfc`, `*.kfc_data`, `*.kfc_dir`, `*.ksp`, `*.rip` — Keen engine archives
* `portal_knights_x64.exe`, `pk_dedicated_server.exe` — game executables
* `steam_api64.dll`, `dedicated_server.zip` — shipped libraries
* `portal_knights.cfg`, `portal_knights.log`, `*.dmp` — files the game writes
* Steam wrapper configuration files kept beside the game

If you keep research material in the project directory, put it in `Reference/` or
`extracted_reference/`, or any path matching the patterns above, and it will stay
out of version control automatically.

> If you ever need to commit a short excerpt for documentation purposes (for example
> a few disassembled instructions quoted in a write-up), keep it to a handful of
> lines and make sure it does not reproduce a substantial part of the game.

---

## 简体中文

### 1. 编译到底需要什么

| 需求 | 说明 |
| --- | --- |
| `x86_64-w64-mingw32-g++` / `-gcc` | MinGW-w64 交叉编译器（Linux/WSL） |
| `make` | 驱动构建 |
| 本仓库自己的源码 | `loader/`、`mods/ime_fix/`、`sdk/` |

就这些。**不需要任何游戏文件，也没有链接任何游戏符号。**

### 2. 为什么不需要游戏文件

* **加载器**（`dinput8.dll`）是一个纯粹的**转发器**：它在运行期按完整路径加载真正的
  系统 `C:\Windows\System32\dinput8.dll`，并把它的 6 个导出原样转发。它不导入任何
  游戏符号。
* **`PK_ImeFix`** 挂钩的是 **Windows API 函数**，而不是游戏函数 —— 例如
  `PeekMessageW/A`、`GetMessageW/A`、`GetAsyncKeyState`、`ImmAssociateContext`
  以及游戏窗口的消息循环。它**不包含任何硬编码的游戏 RVA / 偏移**，因此编译期不需要
  可执行文件（运行期也不绑定某个特定游戏版本）。
* 加载器的插件扫描（`loader/mod_manager.cpp`）在运行期从宿主可执行文件所在目录解析，
  同样没有编译期依赖。

### 3. 运行期需要的东西（**不是**编译期）

复制到游戏根目录、与 `portal_knights_x64.exe` 同级：

| 文件 | 要求 |
| --- | --- |
| `dinput8.dll` | 必须与游戏主程序同目录；原因见下 |
| `mods/plugins/*.dll` | 由加载器从游戏目录加载 |

加载器只有在游戏进程**真的加载了它**时才会启动。由于它被命名为 `dinput8.dll`，
这就要求游戏主程序导入了 `DINPUT8.dll → DirectInput8Create`，且 `DINPUT8`
**不是** KnownDLL。`portal_knights_x64.exe` 两条都满足 —— 这正是加载器以这个名字
发布、而不是随便换个 DLL 名字的原因。

### 4. 可选：自己验证这个前提

如果你想在**自己的**游戏副本上验证加载器的前提，本地只需要
`portal_knights_x64.exe`，然后看它的导入表：

```bash
# Linux / WSL，需已安装 MinGW-w64 或 binutils
x86_64-w64-mingw32-objdump -p "portal_knights_x64.exe" | grep "DLL Name"
```

列表里应该能看到 `DINPUT8.dll`（顺带也能看到游戏还导入了哪些系统模块）。

这一步完全是可选的，只为你自己核对。

### 5. 本地研究用的游戏副本**绝对不要提交**

做逆向分析时，把一份游戏副本放在项目旁边是很正常的。但
**《传送门骑士》是商业版权软件，其中任何内容都不得提交到本仓库。**

根目录的 `.gitignore` 已经强制保证了这一点，它会排除（包括但不限于）：

* `Reference/` —— 本地完整游戏副本（可能有好几 GB）
* `extracted_reference/` —— 解包出来用于分析的游戏文件
* `*.kfc`、`*.kfc_data`、`*.kfc_dir`、`*.ksp`、`*.rip` —— Keen 引擎归档
* `portal_knights_x64.exe`、`pk_dedicated_server.exe` —— 游戏可执行文件
* `steam_api64.dll`、`dedicated_server.zip` —— 随游戏分发的库
* `portal_knights.cfg`、`portal_knights.log`、`*.dmp` —— 游戏运行时写出的文件
* 与游戏放在一起的 Steam 包装层配置文件

如果你要把研究材料放在项目目录里，放进 `Reference/`、`extracted_reference/`，
或任何匹配上述规则的路径，就会自动被版本控制忽略。

> 如果确实需要为文档目的提交一小段摘录（例如在说明文章里引用几条反汇编指令），
> 请控制在几行以内，并确保没有复制游戏的实质性部分。
