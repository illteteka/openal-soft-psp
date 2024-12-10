/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2007 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include "config.h"

#include <stdlib.h>

#include "alMain.h"
#include "alThunk.h"


#if defined(_WIN32)

typedef struct {
    ALuint (*func)(ALvoid*);
    ALvoid *ptr;
    HANDLE thread;
} ThreadInfo;

static DWORD CALLBACK StarterFunc(void *ptr)
{
    ThreadInfo *inf = (ThreadInfo*)ptr;
    ALint ret;

    ret = inf->func(inf->ptr);
    ExitThread((DWORD)ret);

    return (DWORD)ret;
}

ALvoid *StartThread(ALuint (*func)(ALvoid*), ALvoid *ptr)
{
    DWORD dummy;
    ThreadInfo *inf = malloc(sizeof(ThreadInfo));
    if(!inf) return 0;

    inf->func = func;
    inf->ptr = ptr;

    inf->thread = CreateThread(NULL, 0, StarterFunc, inf, 0, &dummy);
    if(!inf->thread)
    {
        free(inf);
        return NULL;
    }

    return inf;
}

ALuint StopThread(ALvoid *thread)
{
    ThreadInfo *inf = thread;
    DWORD ret = 0;

    WaitForSingleObject(inf->thread, INFINITE);
    GetExitCodeThread(inf->thread, &ret);
    CloseHandle(inf->thread);

    free(inf);

    return (ALuint)ret;
}

#elif defined(PSP)

#include <pspkernel.h>
#include <pspdebug.h>

typedef struct {
    ALuint (*func)(ALvoid*);
    ALvoid *ptr;
    ALuint ret;
    int thread;
} ThreadInfo;

static int StarterFunc(SceSize args, void *ptr)
{
    ThreadInfo *inf = *(ThreadInfo **)ptr;
    inf->ret = inf->func(inf->ptr);

	sceKernelExitDeleteThread(0);
    return 0;
}

ALvoid *StartThread(ALuint (*func)(ALvoid*), ALvoid *ptr)
{
    ThreadInfo *inf = malloc(sizeof(ThreadInfo));
    if(!inf) return NULL;

    inf->func = func;
    inf->ptr = ptr;
    inf->thread = sceKernelCreateThread("openal-soft", StarterFunc, 0x16, 0x1800, PSP_THREAD_ATTR_USER, NULL);
    if(inf->thread < 0)
    {
        free(inf);
        return NULL;
    }
    sceKernelStartThread(inf->thread, sizeof(void *), (void *)&inf);

    return inf;
}

ALuint StopThread(ALvoid *thread)
{
    ThreadInfo *inf = thread;
    ALuint ret;

    sceKernelWaitThreadEnd(inf->thread, NULL);
    ret = inf->ret;

    free(inf);

    return ret;
}

#else

#include <pthread.h>

typedef struct {
    ALuint (*func)(ALvoid*);
    ALvoid *ptr;
    ALuint ret;
    pthread_t thread;
} ThreadInfo;

static void *StarterFunc(void *ptr)
{
    ThreadInfo *inf = (ThreadInfo*)ptr;
    inf->ret = inf->func(inf->ptr);
    return NULL;
}

ALvoid *StartThread(ALuint (*func)(ALvoid*), ALvoid *ptr)
{
    ThreadInfo *inf = malloc(sizeof(ThreadInfo));
    if(!inf) return NULL;

    inf->func = func;
    inf->ptr = ptr;
    if(pthread_create(&inf->thread, NULL, StarterFunc, inf) != 0)
    {
        free(inf);
        return NULL;
    }

    return inf;
}

ALuint StopThread(ALvoid *thread)
{
    ThreadInfo *inf = thread;
    ALuint ret;

    pthread_join(inf->thread, NULL);
    ret = inf->ret;

    free(inf);

    return ret;
}

#endif
