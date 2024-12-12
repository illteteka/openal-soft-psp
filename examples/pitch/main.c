/*
Copyright (C) 2024 Ill Teteka

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, see
<https://www.gnu.org/licenses/>.
*/

#include <pspkernel.h>
#include <pspctrl.h>
#include <math.h>

#include <AL/al.h>
#include <AL/alc.h>

#include "piano_c5.h"

PSP_MODULE_INFO("OpenAL Soft example", 0, 1, 1);

int running = 1;

// Input handling

#define _OFF 0
#define _ON 1
#define _PRESS 2
#define _RELEASE 3

int left_key = _OFF;
int right_key = _OFF;
int cross_key = _OFF;
int start_key = _OFF;

int inputStep(int a, int b)
{
	int output = b;

	if (a)
	{
		if (b == _OFF || b == _RELEASE)
			output = _PRESS;
		else if (b == _PRESS)
			output = _ON;
	}
	else
	{
		if (b == _ON || b == _PRESS)
			output = _RELEASE;
		else if (b == _RELEASE)
			output = _OFF;
	}

	return output;
}

// End input handling

void loadWavExample(const unsigned char* wav_data, unsigned int wav_length, ALuint* buffer)
{
	const unsigned char* audio_data = wav_data + 44;
	unsigned int audio_length = wav_length - 44;

	uint32_t sampleRate = 44100;
	ALenum format = AL_FORMAT_MONO16;

	alGenBuffers(1, buffer);
	alBufferData(*buffer, format, audio_data, audio_length - 1600, sampleRate);
}

int main(void)
{
	SceCtrlData pad;

	pspDebugScreenInit();

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	// Set up OpenAL

	ALCdevice* device = alcOpenDevice(NULL);
	ALCcontext* context = alcCreateContext(device, NULL);
	alcMakeContextCurrent(context);

	// Load the wav data
	ALuint buffer;
	loadWavExample(piano_c5_wav, piano_c5_wav_len, &buffer);

	ALuint source;
	alGenSources(1, &source);
	alSourcei(source, AL_BUFFER, buffer);

	float pitch = 1.0f;

	while (running)
	{
		pspDebugScreenSetXY(0, 2);

		sceCtrlReadBufferPositive(&pad, 1);

		pspDebugScreenPrintf("  OpenAL Soft test, start to exit\n");
		pspDebugScreenPrintf("  Use left and right to change pitch\n");
		pspDebugScreenPrintf("  Press (X) to play a sound\n\n");
		pspDebugScreenPrintf("  Current pitch: %f\n", pitch);

		left_key = inputStep((pad.Buttons & PSP_CTRL_LEFT), left_key);
		right_key = inputStep((pad.Buttons & PSP_CTRL_RIGHT), right_key);
		cross_key = inputStep((pad.Buttons & PSP_CTRL_CROSS), cross_key);
		start_key = inputStep((pad.Buttons & PSP_CTRL_START), start_key);

		if (start_key == _PRESS)
			running = 0;

		if (left_key == _ON)
			pitch = fmax(0.01f, pitch - 0.03f);

		if (right_key == _ON)
			pitch += 0.03f;

		if (cross_key == _PRESS)
		{
			alSourceStop(source);
			alSourcef(source, AL_PITCH, pitch);
			alSourcePlay(source);
		}
	}

	alDeleteBuffers(1, &buffer);
	alDeleteSources(1, &source);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);

	return 0;
}