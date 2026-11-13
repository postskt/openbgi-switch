/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * time.c - Clocks, Sleep and Timers for Nintendo Switch
 */
#include "switch_internal.h"

uint32_t OS_TicksMs(void)
{
#ifdef __SWITCH__
	return (uint32_t)(armTicksToNs(armGetSystemTick()) / 1000000ULL);
#else
	return (uint32_t)SDL_GetTicks();
#endif
}

void OS_SleepMs(uint32_t ms)
{
	SDL_Delay(ms);
}

void OS_TimerBeginPeriod(void)
{
}

void OS_TimerEndPeriod(void)
{
}

void OS_LocalTime(OsLocalTime_t* t)
{
	if(!t)
		return;

	time_t now = time(NULL);
	struct tm tm;
	localtime_r(&now, &tm);

	t->year = (uint16_t)(tm.tm_year + 1900);
	t->month = (uint16_t)(tm.tm_mon + 1);
	t->dayOfWeek = (uint16_t)tm.tm_wday;
	t->day = (uint16_t)tm.tm_mday;
	t->hour = (uint16_t)tm.tm_hour;
	t->minute = (uint16_t)tm.tm_min;
	t->second = (uint16_t)tm.tm_sec;
	t->millis = (uint16_t)(OS_TicksMs() % 1000);
}

void OS_IdleInit(void)
{
}

void OS_IdleShutdown(void)
{
}

void OS_IdleSleep(void)
{
	SDL_Delay(1);
}

void OS_IdleSleepMs(int ms)
{
	if(ms <= 0)
		return;
	if(ms > 10)
		ms = 10;
	SDL_Delay((Uint32)ms);
}
