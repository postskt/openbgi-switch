/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * audio.c - SDL2 Streaming Audio Output, MCI and Movie Stubs for Nintendo Switch
 */
#include "switch_internal.h"

static void AudioCallback(void* userdata, Uint8* stream, int len)
{
	BGI_UNUSED(userdata);
	int frames = len / (2 * (int)sizeof(int16_t));
	if(gSwitch.audioFill)
	{
		gSwitch.audioFill(gSwitch.audioUser, (int16_t*)stream, frames);
	}
	else
	{
		memset(stream, 0, (size_t)len);
	}
}

int OS_AudioAvailable(void)
{
	return 1;
}

int OS_AudioStart(int rate, OsAudioFill_t fill, void* user)
{
	if(gSwitch.audioStarted)
		OS_AudioStop();

	if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		fprintf(stderr, "SDL_InitSubSystem(AUDIO) failed: %s\n", SDL_GetError());
		return 0;
	}

	SDL_AudioSpec wanted, have;
	memset(&wanted, 0, sizeof(wanted));
	wanted.freq = rate > 0 ? rate : 44100;
	wanted.format = AUDIO_S16SYS;
	wanted.channels = 2;
	wanted.samples = 1024;
	wanted.callback = AudioCallback;
	wanted.userdata = NULL;

	gSwitch.audioDev = SDL_OpenAudioDevice(NULL, 0, &wanted, &have, 0);
	if(gSwitch.audioDev == 0)
	{
		/* 允许频率协商重试 */
		gSwitch.audioDev = SDL_OpenAudioDevice(NULL, 0, &wanted, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	}

	if(gSwitch.audioDev == 0)
	{
		fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		return 0;
	}

	gSwitch.audioFill = fill;
	gSwitch.audioUser = user;
	gSwitch.audioStarted = true;

	/* 开始拉取音频流 */
	SDL_PauseAudioDevice(gSwitch.audioDev, 0);
	return 1;
}

void OS_AudioStop(void)
{
	if(!gSwitch.audioStarted)
		return;

	gSwitch.audioStarted = false;
	gSwitch.audioFill = NULL;
	gSwitch.audioUser = NULL;

	if(gSwitch.audioDev != 0)
	{
		SDL_PauseAudioDevice(gSwitch.audioDev, 1);
		SDL_CloseAudioDevice(gSwitch.audioDev);
		gSwitch.audioDev = 0;
	}
}

/* MCI CD 音频存根 (Switch 无物理光驱) */
uint32_t OS_MciOpenCdAudio(const char* element)
{
	BGI_UNUSED(element);
	return 0;
}

int OS_MciSetTimeFormatTmsf(uint32_t dev)
{
	BGI_UNUSED(dev);
	return 0;
}

int OS_MciClose(uint32_t dev)
{
	BGI_UNUSED(dev);
	return 0;
}

int OS_MciStatus(uint32_t dev, uint32_t item, uint32_t* out)
{
	BGI_UNUSED(dev);
	BGI_UNUSED(item);
	BGI_UNUSED(out);
	return 0;
}

int OS_MciPlay(uint32_t dev, uint32_t from, uint32_t to, int notify)
{
	BGI_UNUSED(dev);
	BGI_UNUSED(from);
	BGI_UNUSED(to);
	BGI_UNUSED(notify);
	return 0;
}

int OS_MciStop(uint32_t dev)
{
	BGI_UNUSED(dev);
	return 0;
}

int OS_PlaySoundFile(const char* path)
{
	BGI_UNUSED(path);
	return 0;
}

/* 视频播放存根 (返回失败使引擎优雅跳过或使用纯立绘/CG替代) */
int OS_MovieOpen(const char* path)
{
	BGI_UNUSED(path);
	return OS_MOVIE_FAILED;
}

int OS_MovieStart(int x, int y, int w, int h, int32_t volume, int32_t* lengthMs)
{
	BGI_UNUSED(x);
	BGI_UNUSED(y);
	BGI_UNUSED(w);
	BGI_UNUSED(h);
	BGI_UNUSED(volume);
	BGI_UNUSED(lengthMs);
	return 0;
}

void OS_MovieStop(void)
{
}

int OS_MovieIsRunning(void)
{
	return 0;
}

void OS_MovieSetVolume(int32_t vol)
{
	BGI_UNUSED(vol);
}
