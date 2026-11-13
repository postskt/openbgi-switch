/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * native.c - Native file operations and worker pool for Nintendo Switch
 */
#include "switch_internal.h"

void OS_NativeToUtf8(const char* native, char* out, size_t n)
{
	snprintf(out, n, "%s", native ? native : "");
}

void OS_Utf8ToNative(const char* utf8, char* out, size_t n)
{
	snprintf(out, n, "%s", utf8 ? utf8 : "");
}

FILE* OS_NativeOpen(const char* path, const char* mode)
{
	return fopen(path, mode);
}

uint8_t* OS_ReadNativeFile(const char* path, uint32_t* size)
{
	if(size)
		*size = 0;
	FILE* f = fopen(path, "rb");
	if(!f)
		return NULL;

	struct stat st;
	if(fstat(fileno(f), &st) != 0 || st.st_size < 0)
	{
		fclose(f);
		return NULL;
	}

	uint32_t sz = (uint32_t)st.st_size;
	uint8_t* buf = (uint8_t*)malloc(sz + 1);
	if(!buf)
	{
		fclose(f);
		return NULL;
	}

	size_t r = fread(buf, 1, sz, f);
	fclose(f);
	buf[r] = 0;
	if(size)
		*size = (uint32_t)r;
	return buf;
}

int OS_WriteNativeFile(const char* path, const void* data, uint32_t size)
{
	FILE* f = fopen(path, "wb");
	if(!f)
		return 0;
	size_t w = fwrite(data, 1, size, f);
	fclose(f);
	return w == size;
}

int OS_NativeStat(const char* path, OsNativeStat_t* out)
{
	struct stat st;
	if(stat(path, &st) != 0)
		return OS_NATIVE_NONE;
	if(out)
	{
		out->size = (uint64_t)st.st_size;
		out->mtime = (int64_t)st.st_mtime * 1000;
	}
	return S_ISDIR(st.st_mode) ? OS_NATIVE_DIR : OS_NATIVE_FILE;
}

int OS_NativeMkdir(const char* path)
{
	if(mkdir(path, 0755) == 0)
		return 1;
	return OS_NativeStat(path, NULL) == OS_NATIVE_DIR;
}

int OS_NativeRemove(const char* path)
{
	return unlink(path) == 0 || OS_NativeStat(path, NULL) == OS_NATIVE_NONE;
}

int OS_NativeList(const char* dir, OsNativeListFn fn, void* ctx)
{
	DIR* d = opendir(dir[0] ? dir : ".");
	struct dirent* e;
	int count = 0;
	if(!d)
		return -1;
	while((e = readdir(d)) != NULL)
	{
		char path[PATH_MAX];
		if(strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
			continue;
		snprintf(path, sizeof(path), "%s/%s", dir[0] ? dir : ".", e->d_name);
		fn(ctx, e->d_name, OS_NativeStat(path, NULL) == OS_NATIVE_DIR);
		count++;
	}
	closedir(d);
	return count;
}

int OS_NativeSeek(FILE* f, uint64_t pos)
{
	return fseeko(f, (off_t)pos, SEEK_SET) == 0;
}

void OS_NativeAbsolute(const char* path, char* out, size_t n)
{
	snprintf(out, n, "%s", path);
}

int OS_WorkerLimit(void)
{
	return 1;
}

int OS_RunWorkers(int n, int (*fn)(int index, void* ctx), void* ctx)
{
	int failed = 0;
	for(int i = 0; i < n; i++)
	{
		if(fn(i, ctx) != 0)
			failed++;
	}
	return failed;
}

void OS_NativeArgs(int* argc, char*** argv)
{
	BGI_UNUSED(argc);
	BGI_UNUSED(argv);
}
