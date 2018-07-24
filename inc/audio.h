// Methods relating to audio playback

#include "joystick.h"

// Playback sounds in real time, allowing multiple simultaneous wave files
// to be mixed together and played without jitter.
#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#define AUDIO_MAX_VOLUME 100

extern pthread_mutex_t audioMutex;
extern pthread_cond_t stopCond;

typedef struct {
	char* filename;
	_Bool* pStop;
} Audio_threadInput;

// init() must be called before any other functions,
// cleanup() must be called last to stop playback threads and free memory.
void Audio_init(unsigned int numChannels, unsigned int sampleRate);
void Audio_cleanup(void);

// Playback thread functions for wav/mp3.
// ptr must point to an Audio_threadInput
void* Audio_playWAV(void* ptr);
void* Audio_playMP3(void* ptr);

// Get/set the volume.
// setVolume() function posted by StackOverflow user "trenki" at:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
int  Audio_getVolume();
void Audio_setVolume(int newVolume);

// Pause/start audio playback.
void Audio_setPause(_Bool newVal);
_Bool Audio_getPause(void);

// Alerts the main thread to new joystick input
void Audio_setJoystickInput(Joystick_Input input);

#endif
