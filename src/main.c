#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "audio.h"
#include "joystick.h"
#include "pot.h"

#define SOURCE_FILE "wave-files/a2002011001-e02.wav"

int main(void)
{
	printf("Beginning play-back of %s\n", SOURCE_FILE);// Load wave file we want to play:
	wavedata_t sampleFile;
	Audio_readWaveFileIntoMemory(SOURCE_FILE, &sampleFile);

	// Get filenames
	// filenames = getFilenames(WAVE_FILE_DIR, &filenameCount);
	// char filenameBuffer[100];
	// strcpy(filenameBuffer, WAVE_FILE_DIR);
	// strcpy(filenameBuffer + strlen(WAVE_FILE_DIR), filenames[11]);

	// Configure Output Device
	Audio_init(sampleFile.numChannels, sampleFile.sampleRate);
	Joystick_init();
	POT_init();

	// Play Audio
	Audio_queueSound(&sampleFile);

	// Wait until stop
	pthread_mutex_lock(&audioMutex);
	pthread_cond_wait(&stopCond, &audioMutex);
	pthread_mutex_unlock(&audioMutex);


	// Cleanup, letting the music in buffer play out (drain), then close and free.
	Audio_cleanup();
	Joystick_cleanup();
	POT_cleanup();

	printf("Done!\n");
	return 0;
}