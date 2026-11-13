/*
 * This file is part of OpenBGI (https://openbgi.net).
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * thread.c - Pthread, Mutex and Atomics implementation for Nintendo Switch
 */
#include "switch_internal.h"

struct OsLock
{
	pthread_mutex_t m;
};

OsLock_t* OS_LockCreate(void)
{
	OsLock_t* l = (OsLock_t*)malloc(sizeof(*l));
	if(!l)
		return NULL;

	pthread_mutexattr_t a;
	pthread_mutexattr_init(&a);
	pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&l->m, &a);
	pthread_mutexattr_destroy(&a);
	return l;
}

void OS_LockDestroy(OsLock_t* l)
{
	if(!l)
		return;
	pthread_mutex_destroy(&l->m);
	free(l);
}

void OS_LockEnter(OsLock_t* l)
{
	if(l)
		pthread_mutex_lock(&l->m);
}

void OS_LockLeave(OsLock_t* l)
{
	if(l)
		pthread_mutex_unlock(&l->m);
}

struct OsThread
{
	pthread_t t;
	OsThreadFn_t fn;
	void* arg;
};

static void* ThreadEntry(void* p)
{
	OsThread_t* t = (OsThread_t*)p;
	if(t && t->fn)
		t->fn(t->arg);
	return NULL;
}

OsThread_t* OS_ThreadStart(OsThreadFn_t fn, void* arg, int priority)
{
	BGI_UNUSED(priority);
	OsThread_t* t = (OsThread_t*)calloc(1, sizeof(*t));
	if(!t)
		return NULL;

	t->fn = fn;
	t->arg = arg;

	if(pthread_create(&t->t, NULL, ThreadEntry, t) != 0)
	{
		free(t);
		return NULL;
	}
	return t;
}

void OS_ThreadJoin(OsThread_t* t)
{
	if(!t)
		return;
	pthread_join(t->t, NULL);
	free(t);
}

int OS_CpuCount(void)
{
	/* Switch Horizon OS 划分给用户态应用程序 3 个 CPU 核心 */
	return 3;
}

int32_t OS_AtomicInc(volatile int32_t* p)
{
	return __sync_add_and_fetch(p, 1);
}

int32_t OS_AtomicDec(volatile int32_t* p)
{
	return __sync_sub_and_fetch(p, 1);
}
