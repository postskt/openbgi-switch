# OpenBGI Nintendo Switch 移植指南与架构规范

本项目为 OpenBGI（BGI / Ethornell 引擎开源重实现）针对 Nintendo Switch 平台的官方移植层适配。

---

## 一、 移植层架构设计

Nintendo Switch 平台层代码集中位于 `src/os/switch/`，严格遵循 `inc/bgi/os.h` 的接口抽象规范：

| 模块文件 | 功能职责与技术实现 |
| :--- | :--- |
| `switch_internal.h` | 平台全局上下文（`SwitchContext_t`）、硬件常量、头文件统一管理 |
| `process.c` | 生命周期管理：`romfsInit()` / `romfsExit()`，只读资产与持久化存档路径设置 |
| `paths.c` | 路径重定向：Windows 风格路径规范化，资产优先从 `romfs:/` 读取，写入/存档安全重定向到 `sdmc:/switch/openbgi/saves/`，并支持大小写不敏感匹配 |
| `files.c` | 标准 POSIX 文件 I/O 与目录遍历 (`OsFind_t`)，虚拟 `C:\` 驱动器模拟 |
| `window.c` | 原生 720p（1280x720）加速渲染管线：`SDL_RenderSetLogicalSize` 自动黑边居中缩放、流式纹理刷新与 VSync 呈现 |
| `events.c` | 事件循环：Joy-Con/Pro 手柄映射（A确认/左键、B取消/右键、X自动、Y历史、R快进、摇杆光标）与全屏触摸屏手势 |
| `audio.c` | SDL2 16-bit 立体声低延迟音频流拉取（`SDL_OpenAudioDevice` 回调） |
| `font.c` | FreeType2 字体光栅化，自动识别 `romfs:/font.ttf` 或 Switch 系统共享字库（`PlSharedFontType_Standard`） |
| `sjis.c` | Shift-JIS (CP932) 与 UTF-8 互转（支持标准 iconv 与内置 fallback） |
| `time.c` | 高精度计时（`armTicksToNs`）与休眠调度 |
| `thread.c` | Pthread 递归互斥锁与多线程工作池 |
| `machine.c` | 硬件信息与 Windows XP (NT 5.1) 环境模拟 |
| `registry.c` | 注册表持久化到 `sdmc:/switch/openbgi/saves/registry.txt` |
| `stubs.c` | 消息框（`SDL_ShowSimpleMessageBox`）与多窗口/管道安全存根 |

---

## 二、 编译构建方法

### 1. 前置依赖
需要安装 devkitPro 及 Switch 工具链：
```bash
dkp-pacman -S switch-dev switch-sdl2 switch-freetype switch-libvorbis switch-zlib make
```
并设置系统环境变量：
- Windows: `DEVKITPRO=C:/devkitPro`
- Linux: `DEVKITPRO=/opt/devkitpro`

### 2. 编译 Switch Homebrew (.nro)
在 `openbgi/` 根目录执行：
```bash
make switch
# 或者
make -f Makefile.switch
```
编译成功后，将在 `bin/switch/` 目录下生成：
- `bin/switch/bgi.nro`
- `build_switch/bgi.elf`
- `build_switch/bgi.nacp`

---

## 三、 单 NSP 独立封装（免外挂 SD 卡）

遵循用户规范，实现“数据全部内置进单个 `.nsp`，杜绝分体外挂”：

1. **准备 RomFS 目录**：
   在 `openbgi/romfs/` 下放置游戏的全部解包或原始资产：
   - `system.arc`, `sysprg.arc`, `data*.arc`
   - `font.ttf`（可选，若缺省将自动使用 Switch 系统字库）
   - 其他音画资产

2. **一键生成独立单 NSP**：
   使用提供的脚本 `tools/switch/package_nsp.py`：
   ```bash
   python tools/switch/package_nsp.py \
       --nro bin/switch/bgi.nro \
       --romfs romfs \
       --keys keys.dat \
       --titleid 0100B61000010000 \
       --title "游戏名称" \
       --author "开发商" \
       --icon icon.png
   ```

---

## 四、 控制器键位对应表

| Switch 按键 | 对应 Galgame 行为 / 键盘映射 | 说明 |
| :--- | :--- | :--- |
| **A** | 回车 (`VK_RETURN`) / 鼠标左键 | 确认、推进对话、选项选中 |
| **B** | ESC (`VK_ESCAPE`) / 鼠标右键 | 取消、隐藏对话框、呼出右键菜单 |
| **X** | 'A' 键 (`VK_A`) | 自动播放（Auto Mode） |
| **Y** | PageUp (`VK_PRIOR`) | 文本履历回溯（Log / Backlog） |
| **R / ZR** | Ctrl 键 (`VK_CONTROL`) | 按住快速跳过（Skip / Fast Forward） |
| **Plus (+)** | ESC (`VK_ESCAPE`) | 系统主菜单 / 暂停设置 |
| **左摇杆** | 鼠标光标平滑移动 | 允许手柄模拟鼠标无缝操作 |
| **触控屏** | 点击 / 滑动 | 原生鼠标左键与手势操作 |
