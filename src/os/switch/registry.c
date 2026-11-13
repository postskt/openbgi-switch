/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * registry.c - Windows Registry Emulation for Nintendo Switch (Saved on SD card)
 */
#include "switch_internal.h"

int OS_RegReadString(const char* key, const char* value, char* buf, size_t n)
{
	return OsCommon_RegFileRead(gSwitch.regFile, key, value, buf, n);
}

int OS_RegCreateKey(const char* key, const char* cls)
{
	BGI_UNUSED(cls);
	return 1;
}

int OS_RegWriteString(const char* key, const char* value, const char* s)
{
	return OsCommon_RegFileWrite(gSwitch.regFile, key, value, s);
}

int OS_RegDeleteKey(const char* key)
{
	return OsCommon_RegFileDeleteKey(gSwitch.regFile, key);
}
