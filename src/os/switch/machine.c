/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * machine.c - Machine info, OS version, performance counters and shell stubs for Nintendo Switch
 */
#include "switch_internal.h"

void OS_MemoryStatus(OsMemStatus_t* m)
{
	if(!m)
		return;
	memset(m, 0, sizeof(*m));

	/* Switch 拥有 4GB LPDDR4，提供给自制软件通常 3GB 以上 */
	m->totalPhys = 0xC0000000u; // 3 GB
	m->availPhys = 0x80000000u; // 2 GB
	m->totalPageFile = m->totalPhys;
	m->availPageFile = m->availPhys;
	m->totalVirtual = 0x7ffe0000u;
	m->availVirtual = m->totalVirtual;
	m->loadPercent = 33;
}

void OS_MemoryStatusEx(uint64_t* totalPhys, uint64_t* availPhys)
{
	if(totalPhys)
		*totalPhys = 4ULL * 1024 * 1024 * 1024;
	if(availPhys)
		*availPhys = 3ULL * 1024 * 1024 * 1024;
}

int OS_ComputerName(char* buf, size_t n)
{
	snprintf(buf, n, "Switch");
	return 1;
}

int OS_UserName(char* buf, size_t n)
{
	snprintf(buf, n, "Player");
	return 1;
}

int OS_Cpuid(uint32_t leaf, uint32_t out[4])
{
	BGI_UNUSED(leaf);
	if(out)
		memset(out, 0, 4 * sizeof(uint32_t));
	return 0;
}

uint64_t OS_ReadTsc(void)
{
	return 0;
}

uint32_t OS_CpuSignature(void)
{
	return 0;
}

int OS_PerfCounterNs(int64_t* out)
{
	if(!out)
		return 0;
#ifdef __SWITCH__
	*out = (int64_t)armTicksToNs(armGetSystemTick());
	return 1;
#else
	struct timespec ts;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
	{
		*out = (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
		return 1;
	}
	*out = (int64_t)SDL_GetTicks() * 1000000LL;
	return 1;
#endif
}

int OS_Version(int* platform, int* major, int* minor)
{
	/* 报告 Windows XP (NT 5.1)，使引擎走稳定的标准 NT 代码路径 */
	if(platform) *platform = 2; // VER_PLATFORM_WIN32_NT
	if(major) *major = 5;
	if(minor) *minor = 1;
	return 1;
}

void OS_VersionEx(uint32_t out[4], char* csd, size_t n)
{
	if(out)
	{
		out[0] = 5;
		out[1] = 1;
		out[2] = 2600;
		out[3] = 2;
	}
	if(csd && n > 0)
		snprintf(csd, n, "Service Pack 3");
}

int OS_UiLanguage(void)
{
	/* 默认优先返回日文 0x0411 (Japanese) 以适配 Galgame 原作 */
	return 0x0411;
}

int OS_DisplayAdapter(char* desc, size_t n, uint32_t version[4])
{
	snprintf(desc, n, "NVIDIA Tegra X1 (Maxwell)");
	if(version)
	{
		version[0] = 1;
		version[1] = 0;
		version[2] = 0;
		version[3] = 0;
	}
	return 1;
}

int OS_SpecialFolder(int csidl, char* buf, size_t n)
{
	BGI_UNUSED(csidl);
	snprintf(buf, n, "%s", gSwitch.savesDir);
	return 1;
}

int OS_WindowsDir(char* buf, size_t n)
{
	snprintf(buf, n, "C:\\WINDOWS");
	return 1;
}

int OS_ShellExecute(const char* file, const char* params, const char* dir)
{
	BGI_UNUSED(file);
	BGI_UNUSED(params);
	BGI_UNUSED(dir);
	return 0;
}

int OS_ClipboardSetText(const char* text)
{
	BGI_UNUSED(text);
	return 1;
}

int OS_SetWallpaper(const char* path, int stretch, int tile)
{
	BGI_UNUSED(path);
	BGI_UNUSED(stretch);
	BGI_UNUSED(tile);
	return 0;
}

struct OsProcess
{
	int dummy;
};

OsProcess_t* OS_ProcessStart(const char* cmdLine)
{
	BGI_UNUSED(cmdLine);
	return NULL;
}

void OS_ProcessWaitIdle(OsProcess_t* p)
{
	BGI_UNUSED(p);
}

int OS_ProcessWait(OsProcess_t* p, uint32_t ms)
{
	BGI_UNUSED(p);
	BGI_UNUSED(ms);
	return 0;
}

void OS_ProcessClose(OsProcess_t* p)
{
	BGI_UNUSED(p);
}

int OS_CreateShortcut(const char* linkPath, const char* target)
{
	BGI_UNUSED(linkPath);
	BGI_UNUSED(target);
	return 0;
}

int OS_CreateShortcutArgs(const char* linkPath, const char* target, const char* args)
{
	BGI_UNUSED(linkPath);
	BGI_UNUSED(target);
	BGI_UNUSED(args);
	return 0;
}

void OS_ShellNotifyAssocChanged(void)
{
}
