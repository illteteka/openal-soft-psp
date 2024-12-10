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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "alMain.h"
#include "AL/al.h"
#include "AL/alc.h"

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspaudio.h>
#include <psphprm.h>
#include <malloc.h>


#define PSP_SAMPLE_RATE 44100
#define NUM_SAMPLES 1024
#define MIC_LEVEL 0x1000


typedef struct {
    SceUID          nextBuffer;
    int             channel;
    volatile int    volume;
    volatile int    status;
    volatile int    flip;

    ALvoid *        buffer;
    ALuint          size;
    RingBuffer *    ring;

    volatile int    enabled;
    volatile int    killNow;
    ALvoid *        thread;
} psp_data;


static ALCchar *pspDevice;
static ALCchar *pspDevice_capture;


static int audioOutput(SceSize args, void *argp)
{
    psp_data *data = *(psp_data **)argp;
	ALCshort *playbuf;

	while(data->status != 0xDEADBEEF)
	{
		playbuf = (ALCshort *)((int)data->buffer + (data->flip ? 0 : (data->size>>1)));
		data->flip ^= 1;
		sceKernelSignalSema(data->nextBuffer, 1);
		sceAudioOutputBlocking(data->channel, data->volume, playbuf);
	}

	sceKernelExitDeleteThread(0);
	return 0;
}

static ALuint PspOutProc(ALvoid *ptr)
{
    ALCdevice *pDevice = (ALCdevice*)ptr;
    psp_data *data = (psp_data*)pDevice->ExtraData;
	ALCshort *fillbuf;

    // init PSP audio output
    data->nextBuffer = sceKernelCreateSema("Buffer Empty", 0, 1, 1, 0);
    data->volume = PSP_AUDIO_VOLUME_MAX;
    data->status = 0;
    data->flip = 0;
    data->enabled = 1;
	data->channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, NUM_SAMPLES, PSP_AUDIO_FORMAT_STEREO);
    if(data->channel < 0)
    {
        AL_PRINT("FATAL: Cannot reserve an audio output channel\n");
        return 0;
    }

    int audioThid = sceKernelCreateThread("audioOutput", audioOutput, 0x16, 0x1800, PSP_THREAD_ATTR_USER, NULL);
    if(audioThid < 0)
    {
        AL_PRINT("FATAL: Cannot create audioOutput thread\n");
        return 0;
    }
    sceKernelStartThread(audioThid, sizeof(void *), (void *)&data);

    while(!data->killNow)
    {
        sceKernelWaitSema(data->nextBuffer, 1, NULL);
        fillbuf = (ALCshort *)((int)data->buffer + (data->flip ? (data->size>>1) : 0));

        if (data->enabled)
        {
            SuspendContext(NULL);
            aluMixData(pDevice->Context, (void *)fillbuf, data->size>>1, pDevice->Format);
            ProcessContext(NULL);
        }
		else
		{
			// Audio not active, play silence
			memset((char *)fillbuf, 0, data->size>>1);
		}
    }

    // cleanup PSP audio output
    data->status = 0xDEADBEEF;
    sceKernelDelayThread(500*1000); // give audioOutput thread time to terminate
    sceAudioChRelease(data->channel);
    sceKernelDeleteSema(data->nextBuffer);

    return 0;
}

static ALCboolean psp_open_playback(ALCdevice *device, const ALCchar *deviceName)
{
    psp_data *data;
    ALuint channels;
    ALuint bits;

    device->szDeviceName = pspDevice;

    data = (psp_data*)calloc(1, sizeof(psp_data));
    if (!data)
    {
        AL_PRINT("Couldn't allocate private data!\n");
        return ALC_FALSE;
    }

    bits = aluBytesFromFormat(device->Format) * 8;
    channels = aluChannelsFromFormat(device->Format);
    if (bits != 16)
    {
        AL_PRINT("Bad format: %x, must be 16 bit!\n", device->Format);
        free(data);
        return ALC_FALSE;
    }
    if (channels != 2)
    {
        AL_PRINT("Bad format: %x, must be stereo!\n", device->Format);
        free(data);
        return ALC_FALSE;
    }

    device->Frequency = PSP_SAMPLE_RATE;
    device->UpdateSize = NUM_SAMPLES;
    data->size = NUM_SAMPLES * 2 * 2 * 2; // NUM_SAMPLES * stereo * 16 bit * double-buffered
    data->buffer = (void *)memalign(16, data->size);
    if(!data->buffer)
    {
        AL_PRINT("buffer malloc failed\n");
        free(data);
        return ALC_FALSE;
    }

    device->ExtraData = data;
    data->thread = StartThread(PspOutProc, device);
    if(data->thread == NULL)
    {
        AL_PRINT(" StartThread failed\n");
        device->ExtraData = NULL;
        free(data->buffer);
        free(data);
        return ALC_FALSE;
    }

    return ALC_TRUE;
}

static void psp_close_playback(ALCdevice *device)
{
    psp_data *data = (psp_data*)device->ExtraData;

    data->enabled = 0;
    sceKernelDelayThread(200*1000);
    data->killNow = 1;
	sceKernelSignalSema(data->nextBuffer, 1);
    StopThread(data->thread);

    free(data->buffer);
    free(data);
    device->ExtraData = NULL;
}


static int audioInput(SceSize args, void *argp)
{
    psp_data *data = *(psp_data **)argp;
	ALCshort *recbuf;

	while(data->status != 0xDEADBEEF)
	{
		recbuf = (ALCshort *)((int)data->buffer + (data->flip ? 0 : (data->size>>1)));
		sceAudioInputBlocking(NUM_SAMPLES, data->volume, recbuf);
		data->flip ^= 1;
		sceKernelSignalSema(data->nextBuffer, 1);
		sceKernelDelayThread(10);
	}

	sceKernelExitDeleteThread(0);
	return 0;
}

static ALuint PspInProc(ALvoid *ptr)
{
    ALCdevice *pDevice = (ALCdevice*)ptr;
    psp_data *data = (psp_data*)pDevice->ExtraData;
	ALCshort *recbuf;

    // init PSP audio input
    data->nextBuffer = sceKernelCreateSema("Buffer Full", 0, 0, 1, 0);
    data->status = 0;
    data->flip = 0;
    data->enabled = 0;
	data->channel = sceAudioInputInit(0, MIC_LEVEL, 0);
    if (data->channel < 0)
    {
        AL_PRINT("FATAL: sceAudioInputInit failed!\n");
        return 0;
    }
    sceKernelDelayThread(600*1000);

    int audioThid = sceKernelCreateThread("audioInput", audioInput, 0x12, 0x1800, PSP_THREAD_ATTR_USER, NULL);
    if(audioThid < 0)
    {
        AL_PRINT("FATAL: Cannot create audioInput thread\n");
        return 0;
    }
    sceKernelStartThread(audioThid, sizeof(void *), (void *)&data);

    while(!data->killNow)
    {
        sceKernelWaitSema(data->nextBuffer, 1, NULL);
        recbuf = (ALCshort *)((int)data->buffer + (data->flip ? (data->size>>1) : 0));

        if (data->enabled)
            WriteRingBuffer(data->ring, (const ALubyte *)recbuf, NUM_SAMPLES);
    }

    // cleanup PSP audio input
    data->status = 0xDEADBEEF;
    sceKernelDelayThread(500*1000); // give audioInput thread time to terminate
    sceKernelDeleteSema(data->nextBuffer);

    return 0;
}

static ALCboolean psp_open_capture(ALCdevice *device, const ALCchar *deviceName, ALCuint frequency, ALCenum format, ALCsizei SampleSize)
{
    psp_data *data;
    ALuint channels;
    ALuint bits;

    if (!sceHprmIsMicrophoneExist())
        return ALC_FALSE;

    device->szDeviceName = pspDevice_capture;

    data = (psp_data*)malloc(sizeof(psp_data));
    memset((void *)data, 0, sizeof(psp_data));

    bits = aluBytesFromFormat(format) * 8;
    channels = aluChannelsFromFormat(format);
    if (bits != 16)
    {
        AL_PRINT("Bad format: %x, must be 16 bit!\n", format);
        free(data);
        return ALC_FALSE;
    }
    if (channels != 1)
    {
        AL_PRINT("Bad format: %x, must be mono!\n", format);
        free(data);
        return ALC_FALSE;
    }
    if ((frequency != 44100) && (frequency != 22050) && (frequency != 11025))
    {
        AL_PRINT("Bad frequency: %x, must be 44100, 22050, or 11025 Hz!\n", frequency);
        free(data);
        return ALC_FALSE;
    }
    data->volume = frequency; // put the sample rate in the volume field

    data->ring = CreateRingBuffer(2, SampleSize); // frame is mono 16 bit
    if(!data->ring)
    {
        AL_PRINT("ring buffer create failed\n");
        free(data);
        return ALC_FALSE;
    }

    data->size = NUM_SAMPLES * 2 * 2; // NUM_SAMPLES * 16 bit * double-buffered
    data->buffer = (void *)memalign(16, data->size);
    if(!data->buffer)
    {
        AL_PRINT("buffer malloc failed\n");
        DestroyRingBuffer(data->ring);
        free(data);
        return ALC_FALSE;
    }

    device->ExtraData = data;
    data->thread = StartThread(PspInProc, device);
    if(data->thread == NULL)
    {
        AL_PRINT("StartThread failed\n");
        device->ExtraData = NULL;
        free(data->buffer);
        DestroyRingBuffer(data->ring);
        free(data);
        return ALC_FALSE;
    }

    return ALC_TRUE;
}

static void psp_close_capture(ALCdevice *device)
{
    psp_data *data = (psp_data*)device->ExtraData;

    data->enabled = 0;
    sceKernelDelayThread(200*1000);
    data->killNow = 1;
	sceKernelSignalSema(data->nextBuffer, 1);
    StopThread(data->thread);

    DestroyRingBuffer(data->ring);

    free(data->buffer);
    free(data);
    device->ExtraData = NULL;
}

static void psp_start_capture(ALCdevice *pDevice)
{
    psp_data *data = (psp_data*)pDevice->ExtraData;

    data->enabled = 1;
}

static void psp_stop_capture(ALCdevice *pDevice)
{
    psp_data *data = (psp_data*)pDevice->ExtraData;

    data->enabled = 0;
}

static void psp_capture_samples(ALCdevice *pDevice, ALCvoid *pBuffer, ALCuint lSamples)
{
    psp_data *data = (psp_data*)pDevice->ExtraData;

    if (lSamples <= (ALCuint)RingBufferSize(data->ring))
        ReadRingBuffer(data->ring, pBuffer, lSamples);
    else
        SetALCError(ALC_INVALID_VALUE);
}

static ALCuint psp_available_samples(ALCdevice *pDevice)
{
    psp_data *data = (psp_data*)pDevice->ExtraData;

    return RingBufferSize(data->ring);
}


BackendFuncs psp_funcs = {
    psp_open_playback,
    psp_close_playback,
    psp_open_capture,
    psp_close_capture,
    psp_start_capture,
    psp_stop_capture,
    psp_capture_samples,
    psp_available_samples
};

void alc_psp_init(BackendFuncs *func_list)
{
    *func_list = psp_funcs;

    pspDevice = AppendDeviceList("PSP Output");
    AppendAllDeviceList(pspDevice);

    if (sceHprmIsMicrophoneExist())
        pspDevice_capture = AppendCaptureDeviceList("PSP Capture");
}
