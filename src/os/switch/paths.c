/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * paths.c - Path translation and redirection for Nintendo Switch
 */
#include "switch_internal.h"

/* 递归创建目录 (mkdir -p) */
void OsSwitch_EnsureDir(const char* path)
{
	char tmp[PATH_MAX];
	char* p = NULL;
	size_t len;

	if(!path || !*path)
		return;

	snprintf(tmp, sizeof(tmp), "%s", path);
	len = strlen(tmp);
	if(len > 0 && tmp[len - 1] == '/')
		tmp[len - 1] = 0;

	/* 跳过 "romfs:/" 或 "sdmc:/" 挂载前缀 */
	char* start = strstr(tmp, ":/");
	if(start)
		p = start + 2;
	else if(tmp[0] == '/')
		p = tmp + 1;
	else
		p = tmp;

	while((p = strchr(p, '/')) != NULL)
	{
		*p = 0;
		mkdir(tmp, 0755);
		*p = '/';
		p++;
	}
	mkdir(tmp, 0755);
}

/* 检查是否为大小写不敏感匹配 */
static void MatchCase(const char* dir, char* name)
{
	DIR* d = opendir(dir[0] ? dir : ".");
	struct dirent* e;
	if(!d)
		return;
	while((e = readdir(d)) != NULL)
	{
		if(strcasecmp(e->d_name, name) == 0)
		{
			strncpy(name, e->d_name, PATH_MAX - 1);
			name[PATH_MAX - 1] = '\0';
			break;
		}
	}
	closedir(d);
}

/* 检查路径是否属于存档/写操作范畴 */
static bool IsSavePath(const char* p)
{
	if(!p)
		return false;
	if(strcasestr(p, "save") != NULL)
		return true;
	if(strcasestr(p, ".sav") != NULL || strcasestr(p, ".dat") != NULL)
		return true;
	return false;
}

/*
 * 将 Windows 风格 Shift-JIS 路径转为 Unix/Switch 风格 UTF-8 相对规范路径
 * 剥除 "C:\" 或 "C:/" 驱动器前缀，转 '\' 为 '/'
 */
static void NormalizeWinPath(const char* win, char* relOut, size_t n)
{
	char utf[PATH_MAX * 2];
	OsSwitch_SjisToUtf8(win, utf, sizeof(utf));

	const char* p = utf;
	/* 跳过驱动器盘符 (如 "C:") */
	if(((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z')) && p[1] == ':')
	{
		p += 2;
		if(*p == '\\' || *p == '/')
			p++;
	}
	else if(*p == '\\' || *p == '/')
	{
		p++;
	}

	/* 转换分隔符并去除非法前缀 */
	size_t o = 0;
	while(*p && o + 1 < n)
	{
		char c = *p++;
		if(c == '\\')
			c = '/';
		/* 压缩多余连续斜杠 */
		if(c == '/' && o > 0 && relOut[o - 1] == '/')
			continue;
		relOut[o++] = c;
	}
	relOut[o] = 0;
}

/*
 * 只读打开解析规则：
 * 1. 若在 sdmc saves 目录存在（例如先前保存的存档），优先返回 sdmc 路径；
 * 2. 否则从 romfs:/ 下逐层查找；
 * 3. 逐层做大小写不敏感匹配探测；
 * 4. 若 romfs 中找不到，且 SD 卡存在对应资产文件，回退到 SD 卡资产。
 */
void OsSwitch_Path(const char* win, char* out, size_t n)
{
	char rel[PATH_MAX];
	NormalizeWinPath(win, rel, sizeof(rel));

	/* 1. 先探测是否在可写 saves 目录下 */
	char probeSave[PATH_MAX];
	snprintf(probeSave, sizeof(probeSave), "%s/%s", gSwitch.savesDir, rel);
	struct stat st;
	if(stat(probeSave, &st) == 0)
	{
		snprintf(out, n, "%s", probeSave);
		return;
	}

	/* 2. 构造 romfs 路径并进行层级大小写探测 */
	char cur[PATH_MAX];
	snprintf(cur, sizeof(cur), "%s", gSwitch.exeDir);
	/* 保证 base 尾部有 / */
	size_t baseLen = strlen(cur);
	if(baseLen > 0 && cur[baseLen - 1] != '/')
	{
		strncat(cur, "/", sizeof(cur) - strlen(cur) - 1);
	}

	const char* p = rel;
	while(*p)
	{
		char comp[PATH_MAX];
		size_t c = 0;
		while(*p && *p != '/' && c + 1 < sizeof(comp))
			comp[c++] = *p++;
		comp[c] = 0;
		while(*p == '/')
			p++;
		if(c == 0)
			continue;

		if(strcmp(comp, ".") != 0 && strcmp(comp, "..") != 0)
		{
			char probe[PATH_MAX];
			snprintf(probe, sizeof(probe), "%s%s", cur, comp);
			if(stat(probe, &st) != 0)
			{
				/* 在当前目录下匹配大小写 */
				MatchCase(cur, comp);
			}
		}

		strncat(cur, comp, sizeof(cur) - strlen(cur) - 1);
		if(*p)
			strncat(cur, "/", sizeof(cur) - strlen(cur) - 1);
	}

	snprintf(out, n, "%s", cur);
}

/*
 * 写操作路径重定向：
 * 任何写入操作严禁直接落入只读 romfs:/，全部安全重定向至 sdmc:/switch/openbgi/saves/
 */
void OsSwitch_PathForWrite(const char* win, char* out, size_t n)
{
	char rel[PATH_MAX];
	NormalizeWinPath(win, rel, sizeof(rel));

	snprintf(out, n, "%s/%s", gSwitch.savesDir, rel);

	/* 确保上级目录存在 */
	char dirBuf[PATH_MAX];
	snprintf(dirBuf, sizeof(dirBuf), "%s", out);
	char* slash = strrchr(dirBuf, '/');
	if(slash)
	{
		*slash = 0;
		OsSwitch_EnsureDir(dirBuf);
	}
}

/* 导出给上层或内部使用的原生路径转换 */
void OS_PathNative(const char* path, char* out, size_t n)
{
	OsSwitch_Path(path, out, n);
}
