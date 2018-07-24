#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include "audio.h"
#include "joystick.h"
#include "pot.h"
#include "song_data.h"
#include "led_matrix.h"

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2

int main(void)
{
	ledMatrix_init();
    ledMatrix_splash_screen();
	// Configure Output Device
	Audio_init(NUM_CHANNELS, SAMPLE_RATE);
	Joystick_init();
	POT_init();
	Song_data_init();

	// Get Song List
	/*
		The "song" structure is defined in the led_matrix.h, pass the array to it and loop it and the index of the nextSong which could be next or previous or any song in the list.
		If you want to stay in the same song, just keep looping the method with the same nextSong int.
	*/
	// ledMatrix_song_list(song songList[], int nextSong, int colour); 

	// Play Audio
	ledMatrix_music_track_display("EXO", 1114197, 0); // someone needs to pass me tiotle of song
	ledMatrix_music_timer(100, 19, 1114197);
	ledMatrix_start_music_timer(true);
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
