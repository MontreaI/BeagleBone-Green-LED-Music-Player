#ifndef WAVE_PLAYER_H
#define WAVE_PLAYER_H

#include <alsa/asoundlib.h>
#include <stdbool.h>

typedef struct {
	unsigned int sampleRate;
	unsigned int numChannels;
	unsigned int numSamples;
	short *pData;
} wavedata_t;

_Bool stop;
int currentVolume;

snd_pcm_t *Audio_openDevice();
void Audio_readWaveFileIntoMemory(char *fileName, wavedata_t *pWaveStruct);
void Audio_playFile(snd_pcm_t *handle, wavedata_t *pWaveData);

#endif