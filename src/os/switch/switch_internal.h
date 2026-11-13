/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * switch_internal.h - Internal definitions and context for Nintendo Switch OS backend
 */
#ifndef BGI_OS_SWITCH_INTERNAL_H
#define BGI_OS_SWITCH_INTERNAL_H

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <SDL2/SDL.h>

#include "bgi/os.h"
#include "bgi/os_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <limits.h>
#include <strings.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define SWITCH_SCREEN_WIDTH   1280
#define SWITCH_SCREEN_HEIGHT  720

#define SWITCH_DEFAULT_SAVES_DIR "sdmc:/switch/openbgi/saves"
#define SWITCH_DEFAULT_ROMFS_DIR "romfs:/"

typedef struct SwitchContext
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* screenTexture;
	int texW, texH;
	int clientW, clientH;   // 游戏逻辑分辨率 (如 800x600, 1024x768, 1280x720)
	int modeW, modeH;       // 全屏模式分辨率
	int fullscreen;
	
	SDL_AudioDeviceID audioDev;
	bool audioStarted;
	OsAudioFill_t audioFill;
	void* audioUser;

	bool romfsMounted;
	bool alive;
	bool shown;
	bool active;
	int closeRequested;
	int quitPosted;

	int mouseX, mouseY;
	int lastClickBtn;
	uint32_t lastClickTime;
	int lastClickX, lastClickY;
	uint8_t keys[256];

	OsEventHandlers_t handlers;

	char exeDir[PATH_MAX];    // 只读资产根目录 ("romfs:/" 或本地挂载)
	char savesDir[PATH_MAX];  // 存档与配置存储目录 ("sdmc:/switch/openbgi/saves/")
	char regFile[PATH_MAX];   // 模拟注册表文件路径
} SwitchContext_t;

extern SwitchContext_t gSwitch;

/* 内部辅助函数 */
void OsSwitch_Path(const char* win, char* out, size_t n);
void OsSwitch_PathForWrite(const char* win, char* out, size_t n);
void OsSwitch_SjisToUtf8(const char* sjis, char* out, size_t n);
uint32_t OsSwitch_SjisChar(uint16_t code);
int OsSwitch_UnicodeToSjis(uint32_t cp, char out[2]);
void OsSwitch_EnsureDir(const char* path);
void OsSwitch_WindowPresent(void);
void OsSwitch_FontShutdown(void);

#endif /* BGI_OS_SWITCH_INTERNAL_H */
