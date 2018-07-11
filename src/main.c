#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "audio.h"
#include "joystick.h"
#include "pot.h"
#include "song_data.h"

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define SOURCE_FILE "wave-files/a2002011001-e02.wav"

int main(void)
{
	// Configure Output Device
	Audio_init(NUM_CHANNELS, SAMPLE_RATE);
	Joystick_init();
	POT_init();
	Song_data_init();

	// Play Audio
	// printf("Beginning play-back of %s\n", SOURCE_FILE);	// Load wave file we want to play:
	// wavedata_t sampleFile;
	Song_data_playSong(5);

	// wavedata_t sampleFile;
	// Audio_readWaveFileIntoMemory(songBuffer[18], &sampleFile);
	// Audio_queueSound(&sampleFile);

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