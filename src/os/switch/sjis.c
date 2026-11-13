/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * sjis.c - Shift-JIS / CP932 <-> UTF-8 Character Conversion for Nintendo Switch
 */
#include "switch_internal.h"

#if defined(__has_include)
#if __has_include(<iconv.h>)
#include <iconv.h>
#define BGI_SWITCH_HAVE_ICONV 1
#endif
#endif

#ifdef BGI_SWITCH_HAVE_ICONV
static iconv_t gSjisToUtf8 = (iconv_t)-1;
static iconv_t gUtf8ToSjis = (iconv_t)-1;
#endif

static uint32_t* gSjisTable = NULL;

/* 快速半角假名到 Unicode 映射 (0xA1 .. 0xDF -> U+FF61 .. U+FF9F) */
static inline uint32_t HalfwidthKatakana(uint8_t c)
{
	if(c >= 0xa1 && c <= 0xdf)
		return 0xff61u + (c - 0xa1u);
	return 0;
}

static void InitSjisConverters(void)
{
	if(gSjisTable)
		return;

	gSjisTable = (uint32_t*)calloc(0x10000, sizeof(uint32_t));

#ifdef BGI_SWITCH_HAVE_ICONV
	gSjisToUtf8 = iconv_open("UTF-8", "CP932");
	if(gSjisToUtf8 == (iconv_t)-1)
		gSjisToUtf8 = iconv_open("UTF-8", "SJIS");

	gUtf8ToSjis = iconv_open("CP932", "UTF-8");
	if(gUtf8ToSjis == (iconv_t)-1)
		gUtf8ToSjis = iconv_open("SJIS", "UTF-8");
#endif
}

uint32_t OsSwitch_SjisChar(uint16_t code)
{
	InitSjisConverters();

	if(gSjisTable && gSjisTable[code] != 0)
		return gSjisTable[code];

	uint32_t cp = 0xfffd;

#ifdef BGI_SWITCH_HAVE_ICONV
	if(gSjisToUtf8 != (iconv_t)-1)
	{
		char in[2] = { (char)(code >> 8), (char)code };
		char out[8];
		char* ip = in;
		char* op = out;
		size_t il = 2, ol = sizeof(out);

		iconv(gSjisToUtf8, NULL, NULL, NULL, NULL);
		if(iconv(gSjisToUtf8, &ip, &il, &op, &ol) != (size_t)-1 && il == 0)
		{
			const uint8_t* u = (const uint8_t*)out;
			size_t len = sizeof(out) - ol;
			if(len == 1)
				cp = u[0];
			else if(len == 2)
				cp = ((u[0] & 0x1f) << 6) | (u[1] & 0x3f);
			else if(len == 3)
				cp = ((u[0] & 0x0f) << 12) | ((u[1] & 0x3f) << 6) | (u[2] & 0x3f);
		}
	}
#endif

	if(gSjisTable)
		gSjisTable[code] = cp;

	return cp;
}

uint32_t OS_SjisToCodePoint(uint16_t sjis)
{
	return OsSwitch_SjisChar(sjis);
}

void OsSwitch_SjisToUtf8(const char* sjis, char* out, size_t n)
{
	OsCommon_SjisToUtf8(sjis, out, n, OsSwitch_SjisChar);
}

size_t OS_SjisToUtf8(const char* sjis, char* out, size_t n)
{
	return OsCommon_SjisToUtf8(sjis, out, n, OsSwitch_SjisChar);
}

int OsSwitch_UnicodeToSjis(uint32_t cp, char out[2])
{
	if(cp < 0x80)
	{
		if(out) out[0] = (char)cp;
		return 1;
	}
	if(cp >= 0xff61 && cp <= 0xff9f)
	{
		if(out) out[0] = (char)(0xa1 + (cp - 0xff61));
		return 1;
	}

#ifdef BGI_SWITCH_HAVE_ICONV
	InitSjisConverters();
	if(gUtf8ToSjis != (iconv_t)-1)
	{
		char u8[8];
		int ulen = 0;
		if(cp < 0x800)
		{
			u8[ulen++] = (char)(0xc0 | (cp >> 6));
			u8[ulen++] = (char)(0x80 | (cp & 0x3f));
		}
		else if(cp < 0x10000)
		{
			u8[ulen++] = (char)(0xe0 | (cp >> 12));
			u8[ulen++] = (char)(0x80 | ((cp >> 6) & 0x3f));
			u8[ulen++] = (char)(0x80 | (cp & 0x3f));
		}

		char sBuf[4];
		char* ip = u8;
		char* op = sBuf;
		size_t il = (size_t)ulen, ol = sizeof(sBuf);
		iconv(gUtf8ToSjis, NULL, NULL, NULL, NULL);
		if(iconv(gUtf8ToSjis, &ip, &il, &op, &ol) != (size_t)-1)
		{
			size_t produced = sizeof(sBuf) - ol;
			if(produced > 0 && produced <= 2)
			{
				if(out)
				{
					out[0] = sBuf[0];
					if(produced > 1) out[1] = sBuf[1];
				}
				return (int)produced;
			}
		}
	}
#endif

	return 0;
}

size_t OS_Utf8ToSjis(const char* utf8, char* out, size_t n)
{
	if(!utf8 || !out || n == 0)
		return 0;

	const uint8_t* u = (const uint8_t*)utf8;
	size_t o = 0;

	while(*u && o + 2 < n)
	{
		int len, m;
		uint32_t cp = OsCommon_Utf8SeqLen(u) ? OsCommon_Utf8Decode(u, &len) : (len = 1, (uint32_t)*u);
		m = OsSwitch_UnicodeToSjis(cp, out + o);
		if(m <= 0)
		{
			out[o] = '?';
			m = 1;
		}
		o += (size_t)m;
		u += len;
	}
	out[o] = 0;
	return o;
}
