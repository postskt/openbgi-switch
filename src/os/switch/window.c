/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * window.c - 1280x720 Accelerated SDL2 Rendering Pipeline for Nintendo Switch
 */
#include "switch_internal.h"

struct OsDc
{
	int unused;
};

static OsDc_t gMainDc;
static bool gNeedPresent = false;
static uint32_t* gBackbuffer = NULL;
static int gBackbufferW = 0;
static int gBackbufferH = 0;

/* 计算游戏画面居中到 1280x720 屏幕的缩放与黑边偏移 */
static void GetGameViewport(int* outX, int* outY, int* outW, int* outH, float* outScale)
{
	int gw = gSwitch.clientW > 0 ? gSwitch.clientW : SWITCH_SCREEN_WIDTH;
	int gh = gSwitch.clientH > 0 ? gSwitch.clientH : SWITCH_SCREEN_HEIGHT;

	float scaleX = (float)SWITCH_SCREEN_WIDTH / (float)gw;
	float scaleY = (float)SWITCH_SCREEN_HEIGHT / (float)gh;
	float scale = scaleX < scaleY ? scaleX : scaleY;

	int dw = (int)(gw * scale + 0.5f);
	int dh = (int)(gh * scale + 0.5f);
	int dx = (SWITCH_SCREEN_WIDTH - dw) / 2;
	int dy = (SWITCH_SCREEN_HEIGHT - dh) / 2;

	if(outX) *outX = dx;
	if(outY) *outY = dy;
	if(outW) *outW = dw;
	if(outH) *outH = dh;
	if(outScale) *outScale = scale;
}

/* 屏幕像素坐标转游戏客户区坐标 */
void OsSwitch_ScreenToClient(int* x, int* y)
{
	if(!x || !y)
		return;
	int dx, dy, dw, dh;
	float scale;
	GetGameViewport(&dx, &dy, &dw, &dh, &scale);
	if(scale <= 0.0001f)
		return;

	int cx = (int)((*x - dx) / scale);
	int cy = (int)((*y - dy) / scale);

	if(cx < 0) cx = 0;
	if(cy < 0) cy = 0;
	if(cx >= gSwitch.clientW) cx = gSwitch.clientW - 1;
	if(cy >= gSwitch.clientH) cy = gSwitch.clientH - 1;

	*x = cx;
	*y = cy;
}

/* 游戏客户区坐标转屏幕像素坐标 */
void OsSwitch_ClientToScreen(int* x, int* y)
{
	if(!x || !y)
		return;
	int dx, dy, dw, dh;
	float scale;
	GetGameViewport(&dx, &dy, &dw, &dh, &scale);
	*x = (int)(*x * scale + 0.5f) + dx;
	*y = (int)(*y * scale + 0.5f) + dy;
}

/* 重新分配流式纹理与后备缓冲 */
static void ReallocateTexture(int w, int h)
{
	if(w <= 0 || h <= 0)
		return;

	if(gSwitch.screenTexture && gSwitch.texW == w && gSwitch.texH == h)
		return;

	if(gSwitch.screenTexture)
	{
		SDL_DestroyTexture(gSwitch.screenTexture);
		gSwitch.screenTexture = NULL;
	}

	if(gBackbuffer)
	{
		free(gBackbuffer);
		gBackbuffer = NULL;
	}

	gSwitch.texW = w;
	gSwitch.texH = h;
	gBackbufferW = w;
	gBackbufferH = h;
	gBackbuffer = (uint32_t*)calloc(w * h, sizeof(uint32_t));

	if(gSwitch.renderer)
	{
		gSwitch.screenTexture = SDL_CreateTexture(
			gSwitch.renderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			w, h
		);
		SDL_SetTextureBlendMode(gSwitch.screenTexture, SDL_BLENDMODE_NONE);
	}
}

int OS_WindowCreate(const OsWindowDesc_t* d)
{
	if(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0)
	{
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 0;
	}

	gSwitch.window = SDL_CreateWindow(
		"OpenBGI",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SWITCH_SCREEN_WIDTH, SWITCH_SCREEN_HEIGHT,
		SDL_WINDOW_FULLSCREEN
	);
	if(!gSwitch.window)
	{
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		return 0;
	}

	gSwitch.renderer = SDL_CreateRenderer(
		gSwitch.window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
	);
	if(!gSwitch.renderer)
	{
		fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(gSwitch.window);
		gSwitch.window = NULL;
		return 0;
	}

	/* 锁定 720p 原生逻辑分辨率，自动居中等比黑边缩放 */
	SDL_RenderSetLogicalSize(gSwitch.renderer, SWITCH_SCREEN_WIDTH, SWITCH_SCREEN_HEIGHT);

	if(d && d->clientW > 0 && d->clientH > 0)
	{
		gSwitch.clientW = d->clientW;
		gSwitch.clientH = d->clientH;
	}
	else
	{
		gSwitch.clientW = SWITCH_SCREEN_WIDTH;
		gSwitch.clientH = SWITCH_SCREEN_HEIGHT;
	}

	ReallocateTexture(gSwitch.clientW, gSwitch.clientH);

	gSwitch.alive = true;
	gSwitch.shown = true;
	gSwitch.active = true;

	if(gSwitch.handlers.created)
		gSwitch.handlers.created();

	return 1;
}

void OS_WindowDestroy(void)
{
	if(!gSwitch.alive)
		return;

	if(gSwitch.handlers.destroyed)
		gSwitch.handlers.destroyed();

	if(gSwitch.screenTexture)
	{
		SDL_DestroyTexture(gSwitch.screenTexture);
		gSwitch.screenTexture = NULL;
	}
	if(gBackbuffer)
	{
		free(gBackbuffer);
		gBackbuffer = NULL;
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

	gSwitch.alive = false;
	gSwitch.shown = false;
}

int OS_WindowAlive(void)
{
	return gSwitch.alive ? 1 : 0;
}

void OS_WindowShow(int show)
{
	gSwitch.shown = show ? true : false;
}

void OS_WindowSetTitle(const char* t)
{
	if(!gSwitch.window)
		return;
	if(t && *t)
	{
		char title[512];
		OsCommon_MainTitle(title, sizeof(title), t);
		SDL_SetWindowTitle(gSwitch.window, title);
	}
	else
	{
		SDL_SetWindowTitle(gSwitch.window, "OpenBGI");
	}
}

void OS_WindowMove(int x, int y)
{
	BGI_UNUSED(x);
	BGI_UNUSED(y);
}

void OS_WindowGetRect(int* x, int* y, int* w, int* h)
{
	if(x) *x = 0;
	if(y) *y = 0;
	if(w) *w = SWITCH_SCREEN_WIDTH;
	if(h) *h = SWITCH_SCREEN_HEIGHT;
}

int OS_WindowMinimized(void)
{
	return 0;
}

int OS_WindowActive(void)
{
	return gSwitch.active ? 1 : 0;
}

void OS_WindowMinimize(void)
{
}

void OS_WindowClose(void)
{
	gSwitch.closeRequested = 1;
}

void OS_ScreenSize(int* w, int* h)
{
	if(w) *w = SWITCH_SCREEN_WIDTH;
	if(h) *h = SWITCH_SCREEN_HEIGHT;
}

void OS_FrameMetrics(int* fw, int* fh)
{
	if(fw) *fw = 0;
	if(fh) *fh = 0;
}

int OS_FrameTop(void)
{
	return 0;
}

void OS_ImeStatus(int open)
{
	BGI_UNUSED(open);
}

int OS_DisplaySetMode(int fullscreen, int w, int h, int bpp)
{
	BGI_UNUSED(bpp);
	gSwitch.fullscreen = fullscreen ? 1 : 0;
	if(w > 0 && h > 0)
	{
		gSwitch.clientW = w;
		gSwitch.clientH = h;
		ReallocateTexture(w, h);
	}
	return 1;
}

void OS_DisplayRestore(void)
{
	gSwitch.fullscreen = 1;
}

void OS_WindowSetFullscreen(int modeW, int modeH)
{
	gSwitch.fullscreen = 1;
	if(modeW > 0 && modeH > 0)
	{
		gSwitch.clientW = modeW;
		gSwitch.clientH = modeH;
		ReallocateTexture(modeW, modeH);
	}
}

void OS_WindowSetWindowed(int x, int y, int clientW, int clientH)
{
	BGI_UNUSED(x);
	BGI_UNUSED(y);
	if(clientW > 0 && clientH > 0)
	{
		gSwitch.clientW = clientW;
		gSwitch.clientH = clientH;
		ReallocateTexture(clientW, clientH);
	}
}

void OS_WindowSetPicture(int w, int h)
{
	if(w > 0 && h > 0)
	{
		gSwitch.clientW = w;
		gSwitch.clientH = h;
		ReallocateTexture(w, h);
	}
}

OsDc_t* OS_WindowGetDc(void)
{
	return gSwitch.alive ? &gMainDc : NULL;
}

void OS_WindowReleaseDc(OsDc_t* dc)
{
	BGI_UNUSED(dc);
}

/* 核心像素传输：将源表面 src 拷贝至后备缓冲 */
void OS_BlitToWindow(OsDc_t* dc, int dstX, int dstY, int w, int h, const OsSurface_t* src, int srcX, int srcY)
{
	BGI_UNUSED(dc);
	if(!gSwitch.alive || !src || !src->pixels || !gBackbuffer || w <= 0 || h <= 0)
		return;

	int bw = gBackbufferW;
	int bh = gBackbufferH;

	/* 裁剪越界区域 */
	if(dstX < 0) { w += dstX; srcX -= dstX; dstX = 0; }
	if(dstY < 0) { h += dstY; srcY -= dstY; dstY = 0; }
	if(dstX + w > bw) w = bw - dstX;
	if(dstY + h > bh) h = bh - dstY;
	if(srcX < 0 || srcY < 0 || w <= 0 || h <= 0)
		return;

	const uint8_t* sRow = (const uint8_t*)src->pixels + srcY * src->pitch + srcX * 4;
	uint32_t* dRow = gBackbuffer + dstY * bw + dstX;

	for(int y = 0; y < h; y++)
	{
		memcpy(dRow, sRow, w * sizeof(uint32_t));
		sRow += src->pitch;
		dRow += bw;
	}

	gNeedPresent = true;
	OsSwitch_WindowPresent();
}

/* 拉伸拷贝 */
void OS_StretchToWindow(OsDc_t* dc, int dstX, int dstY, int dstW, int dstH, const OsSurface_t* src, const int* clip)
{
	BGI_UNUSED(dc);
	BGI_UNUSED(clip);
	if(!gSwitch.alive || !src || !src->pixels || !gBackbuffer || dstW <= 0 || dstH <= 0)
		return;

	int bw = gBackbufferW;
	int bh = gBackbufferH;

	/* 最近邻简易拉伸，如尺寸相等则快速全幅拷贝 */
	if(dstW == src->width && dstH == src->height && dstX == 0 && dstY == 0 && bw == dstW && bh == dstH)
	{
		const uint8_t* sRow = (const uint8_t*)src->pixels;
		uint32_t* dRow = gBackbuffer;
		for(int y = 0; y < dstH; y++)
		{
			memcpy(dRow, sRow, dstW * sizeof(uint32_t));
			sRow += src->pitch;
			dRow += bw;
		}
	}
	else
	{
		for(int y = 0; y < dstH; y++)
		{
			int sy = y * src->height / dstH;
			if(sy >= src->height) sy = src->height - 1;
			int dy = dstY + y;
			if(dy < 0 || dy >= bh) continue;

			const uint32_t* sRow = (const uint32_t*)((const uint8_t*)src->pixels + sy * src->pitch);
			uint32_t* dRow = gBackbuffer + dy * bw;

			for(int x = 0; x < dstW; x++)
			{
				int dx = dstX + x;
				if(dx < 0 || dx >= bw) continue;
				int sx = x * src->width / dstW;
				if(sx >= src->width) sx = src->width - 1;
				dRow[dx] = sRow[sx];
			}
		}
	}

	gNeedPresent = true;
	OsSwitch_WindowPresent();
}

void OS_WindowFillBlack(void)
{
	if(gBackbuffer && gBackbufferW > 0 && gBackbufferH > 0)
	{
		memset(gBackbuffer, 0, gBackbufferW * gBackbufferH * sizeof(uint32_t));
	}
	if(gSwitch.renderer)
	{
		SDL_SetRenderDrawColor(gSwitch.renderer, 0, 0, 0, 255);
		SDL_RenderClear(gSwitch.renderer);
		SDL_RenderPresent(gSwitch.renderer);
	}
	gNeedPresent = false;
}

/* 将后备缓冲提交渲染至屏幕 */
void OsSwitch_WindowPresent(void)
{
	if(!gSwitch.alive || !gSwitch.renderer || !gSwitch.screenTexture || !gBackbuffer || !gNeedPresent)
		return;

	SDL_UpdateTexture(gSwitch.screenTexture, NULL, gBackbuffer, gBackbufferW * sizeof(uint32_t));

	SDL_SetRenderDrawColor(gSwitch.renderer, 0, 0, 0, 255);
	SDL_RenderClear(gSwitch.renderer);

	int dx, dy, dw, dh;
	GetGameViewport(&dx, &dy, &dw, &dh, NULL);
	SDL_Rect dstRect = { dx, dy, dw, dh };

	SDL_RenderCopy(gSwitch.renderer, gSwitch.screenTexture, NULL, &dstRect);
	SDL_RenderPresent(gSwitch.renderer);

	gNeedPresent = false;
}
