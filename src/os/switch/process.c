/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * process.c - Process lifecycle, initialization and romfs/sdmc setup for Nintendo Switch
 */
#include "switch_internal.h"

SwitchContext_t gSwitch;

static char gArgv0[PATH_MAX];

void OS_SetArgv0(const char* argv0)
{
	if(argv0)
		strncpy(gArgv0, argv0, sizeof(gArgv0) - 1);
	else
		gArgv0[0] = '\0';
}

int OS_Init(void)
{
	memset(&gSwitch, 0, sizeof(gSwitch));

	/* 1. 初始化 RomFS (Nintendo Switch 虚拟文件系统) */
#ifdef __SWITCH__
	Result rc = romfsInit();
	if(R_SUCCEEDED(rc))
	{
		gSwitch.romfsMounted = true;
		strncpy(gSwitch.exeDir, SWITCH_DEFAULT_ROMFS_DIR, sizeof(gSwitch.exeDir) - 1);
	}
	else
#endif
	{
		/* 非 Switch 或 RomFS 失败时使用当前目录 */
		gSwitch.romfsMounted = false;
		if(!getcwd(gSwitch.exeDir, sizeof(gSwitch.exeDir) - 2))
			strcpy(gSwitch.exeDir, ".");
		strncat(gSwitch.exeDir, "/", sizeof(gSwitch.exeDir) - strlen(gSwitch.exeDir) - 1);
	}

	/* 2. 初始化持久化存储与存档路径 */
	strncpy(gSwitch.savesDir, SWITCH_DEFAULT_SAVES_DIR, sizeof(gSwitch.savesDir) - 1);
	OsSwitch_EnsureDir(gSwitch.savesDir);

	/* 3. 注册表模拟文件 */
	snprintf(gSwitch.regFile, sizeof(gSwitch.regFile), "%s/registry.txt", gSwitch.savesDir);

	/* 4. 默认窗口尺寸与状态 */
	gSwitch.clientW = SWITCH_SCREEN_WIDTH;
	gSwitch.clientH = SWITCH_SCREEN_HEIGHT;
	gSwitch.modeW = SWITCH_SCREEN_WIDTH;
	gSwitch.modeH = SWITCH_SCREEN_HEIGHT;
	gSwitch.fullscreen = 1;
	gSwitch.active = true;
	gSwitch.lastClickBtn = -1;

	gSwitch.alive = true;

	return 1;
}

void OS_Shutdown(void)
{
	OsSwitch_FontShutdown();

	if(gSwitch.screenTexture)
	{
		SDL_DestroyTexture(gSwitch.screenTexture);
		gSwitch.screenTexture = NULL;
	}
	if(gSwitch.renderer)
	{
		SDL_DestroyRenderer(gSwitch.renderer);
		gSwitch.renderer = NULL;
	}
	if(gSwitch.window)
	{
		SDL_DestroyWindow(gSwitch.window);
		gSwitch.window = NULL;
	}

	if(SDL_WasInit(0))
	{
		SDL_Quit();
	}

#ifdef __SWITCH__
	if(gSwitch.romfsMounted)
	{
		romfsExit();
		gSwitch.romfsMounted = false;
	}
#endif

	gSwitch.alive = false;
}

void OS_ExeDir(char* buf, size_t n)
{
	/* OpenBGI 假定主程序位于根盘符，所有资源路径都拼在 "C:\" 之后 */
	snprintf(buf, n, "C:\\");
}

void OS_BinaryDir(char* buf, size_t n)
{
	snprintf(buf, n, "%s", gSwitch.exeDir);
}

void OS_GameDir(char* buf, size_t n)
{
	snprintf(buf, n, "%s", gSwitch.exeDir);
}

int OS_SetGameDir(const char* dir)
{
	if(!dir || !*dir)
		return 0;
	strncpy(gSwitch.exeDir, dir, sizeof(gSwitch.exeDir) - 2);
	size_t len = strlen(gSwitch.exeDir);
	if(len > 0 && gSwitch.exeDir[len - 1] != '/')
	{
		gSwitch.exeDir[len] = '/';
		gSwitch.exeDir[len + 1] = 0;
	}
	return 1;
}

int OS_SetCurrentDir(const char* dir)
{
	BGI_UNUSED(dir);
	return 1;
}

/* 实例锁与互斥体模拟：Switch 沙盒单进程无需真正进程间互斥 */
struct OsMutex
{
	int dummy;
};

OsMutex_t* OS_InstanceMutexCreate(const char* name, int* alreadyExists)
{
	BGI_UNUSED(name);
	if(alreadyExists)
		*alreadyExists = 0;
	static OsMutex_t m;
	return &m;
}

void OS_InstanceMutexRelease(OsMutex_t* m)
{
	BGI_UNUSED(m);
}

int OS_NamedMutexExists(const char* name)
{
	BGI_UNUSED(name);
	return 0;
}
