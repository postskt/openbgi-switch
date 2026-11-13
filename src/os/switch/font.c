/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * font.c - FreeType2 Font Rasterizer & Switch SharedFont Integration for Nintendo Switch
 */
#include "switch_internal.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_SIZES_H
#include FT_SYNTHESIS_H
#include FT_TRUETYPE_TABLES_H

static FT_Library gFt = NULL;
static char gCustomFontPath[PATH_MAX] = {0};
static const void* gCustomFontMem = NULL;
static uint32_t gCustomFontMemSize = 0;

#ifdef __SWITCH__
static PlFontData gSharedFont;
static bool gSharedFontLoaded = false;
#endif

struct OsFont
{
	FT_Face face;
	int height;
	int width;
	int bold;
	int italic;
	int ascent;
	int descent;
};

static bool FtInit(void)
{
	if(gFt)
		return true;
	if(FT_Init_FreeType(&gFt) != 0)
	{
		gFt = NULL;
		return false;
	}

#ifdef __SWITCH__
	Result rc = plInitialize(PlServiceType_User);
	if(R_SUCCEEDED(rc))
	{
		rc = plGetSharedFontByType(&gSharedFont, PlSharedFontType_Standard);
		if(R_SUCCEEDED(rc))
		{
			gSharedFontLoaded = true;
		}
	}
#endif

	return true;
}

void OsSwitch_FontShutdown(void)
{
	if(gFt)
	{
		FT_Done_FreeType(gFt);
		gFt = NULL;
	}
#ifdef __SWITCH__
	if(gSharedFontLoaded)
	{
		plExit();
		gSharedFontLoaded = false;
	}
#endif
}

/* 探测字体文件路径 */
static bool ProbeFont(const char* path, char* out, size_t n)
{
	struct stat st;
	if(path && *path && stat(path, &st) == 0 && S_ISREG(st.st_mode))
	{
		snprintf(out, n, "%s", path);
		return true;
	}
	return false;
}

static bool ResolveFont(char* out, size_t n)
{
	if(gCustomFontPath[0] && ProbeFont(gCustomFontPath, out, n))
		return true;

	/* 依次探测 romfs 和 sdmc 中的常用字体名称 */
	static const char* candidates[] = {
		"romfs:/font.ttf",
		"romfs:/default.ttf",
		"romfs:/font.otf",
		"romfs:/msgothic.ttc",
		"sdmc:/switch/openbgi/font.ttf",
		"sdmc:/switch/openbgi/default.ttf",
		NULL
	};

	for(int i = 0; candidates[i]; i++)
	{
		if(ProbeFont(candidates[i], out, n))
			return true;
	}

	return false;
}

int OS_FontAddFile(const char* path)
{
	if(!path || !*path)
		return 0;
	strncpy(gCustomFontPath, path, sizeof(gCustomFontPath) - 1);
	return 1;
}

int OS_FontAddMemory(const void* data, uint32_t size)
{
	if(!data || size == 0)
		return 0;
	gCustomFontMem = data;
	gCustomFontMemSize = size;
	return 1;
}

int OS_FontFaceExists(const char* face)
{
	BGI_UNUSED(face);
	return 1;
}

int OS_FontEnumFaces(char* buf, int charset, int* outBytes)
{
	BGI_UNUSED(buf);
	BGI_UNUSED(charset);
	if(outBytes)
		*outBytes = 0;
	return 0;
}

OsFont_t* OS_FontOpen(const char* face, int height, int width, int bold, int italic)
{
	BGI_UNUSED(face);
	if(height <= 0 || !FtInit())
		return NULL;

	OsFont_t* f = (OsFont_t*)calloc(1, sizeof(*f));
	if(!f)
		return NULL;

	f->height = height;
	f->width = width > 0 ? width : (height / 2);
	f->bold = bold;
	f->italic = italic;

	char fontPath[PATH_MAX];
	FT_Error err = -1;

	/* 1. 若有内存字体 */
	if(gCustomFontMem && gCustomFontMemSize > 0)
	{
		err = FT_New_Memory_Face(gFt, (const FT_Byte*)gCustomFontMem, (FT_Long)gCustomFontMemSize, 0, &f->face);
	}

	/* 2. 尝试从文件系统加载 */
	if(err != 0 && ResolveFont(fontPath, sizeof(fontPath)))
	{
		err = FT_New_Face(gFt, fontPath, 0, &f->face);
	}

	/* 3. 在 Switch 上尝试加载系统共享字体 (全套中日韩字库) */
#ifdef __SWITCH__
	if(err != 0 && gSharedFontLoaded)
	{
		err = FT_New_Memory_Face(gFt, (const FT_Byte*)gSharedFont.address, (FT_Long)gSharedFont.size, 0, &f->face);
	}
#endif

	if(err != 0 || !f->face)
	{
		free(f);
		return NULL;
	}

	/* 选择 Unicode 字符映射表 */
	if(FT_Select_Charmap(f->face, FT_ENCODING_UNICODE) != 0 && f->face->num_charmaps > 0)
	{
		FT_Set_Charmap(f->face, f->face->charmaps[0]);
	}

	/* 设置像素尺寸 */
	FT_Set_Pixel_Sizes(f->face, 0, f->height);

	/* 计算基线 Ascent / Descent */
	TT_OS2* os2 = (TT_OS2*)FT_Get_Sfnt_Table(f->face, FT_SFNT_OS2);
	if(os2 && (os2->usWinAscent + os2->usWinDescent) > 0)
	{
		int cell = os2->usWinAscent + os2->usWinDescent;
		f->ascent = (int)((int64_t)f->height * os2->usWinAscent / cell);
		f->descent = f->height - f->ascent;
	}
	else
	{
		int cell = f->face->ascender - f->face->descender;
		if(cell > 0)
		{
			f->ascent = (int)((int64_t)f->height * f->face->ascender / cell);
			f->descent = f->height - f->ascent;
		}
		else
		{
			f->ascent = (f->height * 4) / 5;
			f->descent = f->height - f->ascent;
		}
	}

	return f;
}

OsFont_t* OS_FontOpenMono(const char* face, int size, int bold)
{
	return OS_FontOpen(face, size, size / 2, bold, 0);
}

void OS_FontClose(OsFont_t* f)
{
	if(!f)
		return;
	if(f->face)
		FT_Done_Face(f->face);
	free(f);
}

/* 测量字符步进宽度 */
int OS_FontCharAdvance(OsFont_t* f, const char* sjis, int len)
{
	if(!f || !f->face || !sjis || len <= 0)
		return 0;

	uint32_t cp = OsCommon_CodePoint(sjis, len, OsSwitch_SjisChar);
	FT_UInt glyph_index = FT_Get_Char_Index(f->face, cp);
	if(glyph_index == 0)
	{
		/* 缺省宽度：半角/全角估算 */
		return (len == 1) ? f->width : (f->width * 2);
	}

	if(FT_Load_Glyph(f->face, glyph_index, FT_LOAD_DEFAULT) == 0)
	{
		int adv = (int)(f->face->glyph->advance.x >> 6);
		return adv > 0 ? adv : ((len == 1) ? f->width : (f->width * 2));
	}

	return (len == 1) ? f->width : (f->width * 2);
}

/* 光栅化字符至 8-bit 单通道灰度缓冲 */
void OS_FontRender(OsFont_t* f, const char* sjis, int len, uint8_t* buf, int pitch, int w, int h)
{
	if(!f || !f->face || !sjis || len <= 0 || !buf || pitch <= 0 || w <= 0 || h <= 0)
		return;

	uint32_t cp = OsCommon_CodePoint(sjis, len, OsSwitch_SjisChar);
	FT_UInt glyph_index = FT_Get_Char_Index(f->face, cp);
	if(glyph_index == 0)
		return;

	FT_Int32 load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL;
	if(FT_Load_Glyph(f->face, glyph_index, load_flags) != 0)
		return;

	if(f->bold)
	{
		FT_GlyphSlot_Embolden(f->face->glyph);
	}
	if(f->italic)
	{
		FT_GlyphSlot_Oblique(f->face->glyph);
	}

	FT_GlyphSlot slot = f->face->glyph;
	FT_Bitmap* bmp = &slot->bitmap;

	int x0 = slot->bitmap_left;
	int y0 = f->ascent - slot->bitmap_top;

	for(int r = 0; r < (int)bmp->rows; r++)
	{
		int dy = y0 + r;
		if(dy < 0 || dy >= h)
			continue;

		const uint8_t* srcRow = bmp->buffer + r * bmp->pitch;
		uint8_t* dstRow = buf + dy * pitch;

		for(int c = 0; c < (int)bmp->width; c++)
		{
			int dx = x0 + c;
			if(dx < 0 || dx >= w)
				continue;

			uint8_t val = srcRow[c];
			if(val > 0)
			{
				/* 叠加写入灰度/Coverage 值 */
				uint32_t blended = (uint32_t)dstRow[dx] + val;
				dstRow[dx] = blended > 255 ? 255 : (uint8_t)blended;
			}
		}
	}
}

void OS_FontRenderMono(OsFont_t* f, const char* sjis, int len, uint8_t* bits, int pitch, int w, int h)
{
	OS_FontRender(f, sjis, len, bits, pitch, w, h);
}
