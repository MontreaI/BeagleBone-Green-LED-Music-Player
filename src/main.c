#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "audio.h"
#include "joystick.h"
#include "pot.h"
#include "song_data.h"

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2

int main(void)
{
	// Configure Output Device
	Audio_init(NUM_CHANNELS, SAMPLE_RATE);
	Joystick_init();
	POT_init();
	Song_data_init();

	// Play Audio
	Song_data_playSong(1);

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