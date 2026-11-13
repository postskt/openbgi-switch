/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * files.c - File I/O, attributes, directory search and drives for Nintendo Switch
 */
#include "switch_internal.h"

struct OsFile
{
	FILE* f;
	char path[PATH_MAX];
};

OsFile_t* OS_FileOpenRead(const char* path)
{
	OsFile_t* f = (OsFile_t*)calloc(1, sizeof(*f));
	if(!f)
		return NULL;

	struct stat st;
	OsSwitch_Path(path, f->path, sizeof(f->path));
	f->f = fopen(f->path, "rb");

	/* 若打开的是目录，则视作打开失败 */
	if(f->f && fstat(fileno(f->f), &st) == 0 && S_ISDIR(st.st_mode))
	{
		fclose(f->f);
		f->f = NULL;
	}

	if(!f->f)
	{
		free(f);
		return NULL;
	}
	return f;
}

OsFile_t* OS_FileCreate(const char* path)
{
	OsFile_t* f = (OsFile_t*)calloc(1, sizeof(*f));
	if(!f)
		return NULL;

	/* 写入重定向至 sdmc saves 目录 */
	OsSwitch_PathForWrite(path, f->path, sizeof(f->path));
	f->f = fopen(f->path, "w+b");
	if(!f->f)
	{
		free(f);
		return NULL;
	}
	return f;
}

OsFile_t* OS_FileOpenWrite(const char* path)
{
	OsFile_t* f = (OsFile_t*)calloc(1, sizeof(*f));
	if(!f)
		return NULL;

	OsSwitch_PathForWrite(path, f->path, sizeof(f->path));
	f->f = fopen(f->path, "r+b");
	if(!f->f && errno == ENOENT)
		f->f = fopen(f->path, "w+b");

	if(!f->f)
	{
		free(f);
		return NULL;
	}
	return f;
}

void OS_FileClose(OsFile_t* f)
{
	if(!f)
		return;
	if(f->f)
		fclose(f->f);
	free(f);
}

uint32_t OS_FileRead(OsFile_t* f, void* buf, uint32_t n)
{
	if(!f || !f->f || !buf || n == 0)
		return 0;
	size_t r = fread(buf, 1, n, f->f);
	return (uint32_t)r;
}

uint32_t OS_FileWrite(OsFile_t* f, const void* buf, uint32_t n)
{
	if(!f || !f->f || !buf || n == 0)
		return 0;
	size_t w = fwrite(buf, 1, n, f->f);
	return (uint32_t)w;
}

int OS_FileSeek(OsFile_t* f, uint32_t pos)
{
	if(!f || !f->f)
		return 0;
	return fseek(f->f, (long)pos, SEEK_SET) == 0;
}

uint32_t OS_FileSize(OsFile_t* f)
{
	if(!f || !f->f)
		return 0;
	struct stat st;
	if(fstat(fileno(f->f), &st) == 0)
		return (uint32_t)st.st_size;
	return 0;
}

/* Unix time to Windows FILETIME (100ns units since 1601-01-01) */
static void UnixToFileTime(time_t t, uint32_t* lo, uint32_t* hi)
{
	uint64_t ft = ((uint64_t)t + 11644473600ULL) * 10000000ULL;
	if(lo)
		*lo = (uint32_t)ft;
	if(hi)
		*hi = (uint32_t)(ft >> 32);
}

int OS_FileGetTime(OsFile_t* f, uint32_t* lo, uint32_t* hi)
{
	if(!f || !f->f)
		return 0;
	struct stat st;
	if(fstat(fileno(f->f), &st) != 0)
		return 0;
	UnixToFileTime(st.st_mtime, lo, hi);
	return 1;
}

int OS_FileSetTime(OsFile_t* f, uint32_t lo, uint32_t hi)
{
	BGI_UNUSED(f);
	BGI_UNUSED(lo);
	BGI_UNUSED(hi);
	return 1;
}

uint32_t OS_FileAttrs(const char* path)
{
	char p[PATH_MAX];
	struct stat st;
	OsSwitch_Path(path, p, sizeof(p));
	if(stat(p, &st) != 0)
		return OS_INVALID_ATTRS;

	uint32_t a = 0;
	if(S_ISDIR(st.st_mode))
		a |= OS_ATTR_DIRECTORY;
	if(!(st.st_mode & S_IWUSR))
		a |= OS_ATTR_READONLY;
	return a ? a : OS_ATTR_NORMAL;
}

int OS_FileSetAttrs(const char* path, uint32_t attrs)
{
	char p[PATH_MAX];
	struct stat st;
	OsSwitch_Path(path, p, sizeof(p));
	if(stat(p, &st) != 0)
		return 0;
	if(attrs & OS_ATTR_READONLY)
		st.st_mode &= (mode_t) ~(S_IWUSR | S_IWGRP | S_IWOTH);
	else
		st.st_mode |= S_IWUSR;
	return chmod(p, st.st_mode) == 0;
}

int OS_FileDelete(const char* path)
{
	char p[PATH_MAX];
	OsSwitch_Path(path, p, sizeof(p));
	return unlink(p) == 0;
}

int OS_FileMove(const char* from, const char* to)
{
	char a[PATH_MAX], b[PATH_MAX];
	OsSwitch_Path(from, a, sizeof(a));
	OsSwitch_PathForWrite(to, b, sizeof(b));
	return rename(a, b) == 0;
}

int OS_FileCopy(const char* from, const char* to, int failIfExists)
{
	char a[PATH_MAX], b[PATH_MAX];
	char buf[0x10000];
	OsSwitch_Path(from, a, sizeof(a));
	OsSwitch_PathForWrite(to, b, sizeof(b));

	if(failIfExists && access(b, F_OK) == 0)
		return 0;

	FILE* in = fopen(a, "rb");
	if(!in)
		return 0;
	FILE* out = fopen(b, "wb");
	if(!out)
	{
		fclose(in);
		return 0;
	}

	size_t n;
	while((n = fread(buf, 1, sizeof(buf), in)) > 0)
	{
		fwrite(buf, 1, n, out);
	}
	fclose(in);
	fclose(out);
	return 1;
}

int OS_DirCreate(const char* path)
{
	char p[PATH_MAX];
	OsSwitch_PathForWrite(path, p, sizeof(p));
	return mkdir(p, 0755) == 0;
}

int OS_DirRemove(const char* path)
{
	char p[PATH_MAX];
	OsSwitch_PathForWrite(path, p, sizeof(p));
	return rmdir(p) == 0;
}

/* 目录枚举 */
struct OsFind
{
	DIR* dir;
	char dirPath[PATH_MAX];
	char pattern[260];
};

static int FindFill(OsFind_t* h, OsFindData_t* out)
{
	struct dirent* e;
	while((e = readdir(h->dir)) != NULL)
	{
		if(!OsCommon_WildcardMatch(h->pattern, e->d_name))
			continue;

		char full[PATH_MAX];
		struct stat st;
		snprintf(full, sizeof(full), "%s/%s", h->dirPath, e->d_name);
		out->attrs = OS_ATTR_NORMAL;
		out->size = 0;
		if(stat(full, &st) == 0)
		{
			out->attrs = 0;
			if(S_ISDIR(st.st_mode))
				out->attrs |= OS_ATTR_DIRECTORY;
			if(!(st.st_mode & S_IWUSR))
				out->attrs |= OS_ATTR_READONLY;
			if(!out->attrs)
				out->attrs = OS_ATTR_NORMAL;
			out->size = (uint32_t)st.st_size;
			UnixToFileTime(st.st_mtime, &out->timeLo, &out->timeHi);
		}

		/* 将匹配的文件名转为 Shift-JIS 填入 out->name */
		OsSwitch_UnicodeToSjis(0, NULL); // dummy check
		OS_Utf8ToSjis(e->d_name, out->name, sizeof(out->name));
		return 1;
	}
	return 0;
}

OsFind_t* OS_FindFirst(const char* pattern, OsFindData_t* out)
{
	OsFind_t* h = (OsFind_t*)calloc(1, sizeof(*h));
	if(!h)
		return NULL;

	char win[PATH_MAX], utfPat[260];
	const char* file = OsCommon_FilePart(pattern);
	size_t dirLen = (size_t)(file - pattern);
	if(dirLen >= sizeof(win))
		dirLen = sizeof(win) - 1;
	memcpy(win, pattern, dirLen);
	win[dirLen] = 0;

	if(dirLen == 0)
		strcpy(h->dirPath, gSwitch.exeDir);
	else
		OsSwitch_Path(win, h->dirPath, sizeof(h->dirPath));

	OsSwitch_SjisToUtf8(file, utfPat, sizeof(utfPat));
	snprintf(h->pattern, sizeof(h->pattern), "%s", utfPat);

	h->dir = opendir(h->dirPath);
	if(!h->dir || !FindFill(h, out))
	{
		if(h->dir)
			closedir(h->dir);
		free(h);
		return NULL;
	}
	return h;
}

int OS_FindNext(OsFind_t* h, OsFindData_t* out)
{
	if(!h || !h->dir || !out)
		return 0;
	return FindFill(h, out);
}

void OS_FindClose(OsFind_t* h)
{
	if(!h)
		return;
	if(h->dir)
		closedir(h->dir);
	free(h);
}

/* 磁盘驱动器接口 */
uint32_t OS_DriveType(const char* root)
{
	if(!root)
		return OS_DRIVE_UNKNOWN;
	if((root[0] == 'C' || root[0] == 'c') && root[1] == ':')
		return OS_DRIVE_FIXED;
	return OS_DRIVE_NO_ROOT;
}

uint32_t OS_LogicalDrives(char* buf, uint32_t n)
{
	if(!buf || n < 4)
		return 0;
	/* 返回 "C:\" 格式的标准双 NUL 结尾驱动器字符串 */
	buf[0] = 'C';
	buf[1] = ':';
	buf[2] = '\\';
	buf[3] = '\0';
	if(n > 4)
		buf[4] = '\0';
	return 4;
}

int OS_VolumeReady(const char* root)
{
	BGI_UNUSED(root);
	return 1;
}

int OS_MediaPresent(char driveLetter)
{
	return (driveLetter == 'C' || driveLetter == 'c');
}
