#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include "audio.h"
#include "joystick.h"
#include "pot.h"
#include "song_data.h"
#include "led_matrix.h"
#include <unistd.h>

#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2

#define DEFAULT_ROW_OFFSET 0
#define DEFAULT_HORIZONTAL_OFFSET 19

int main(void)
{

	/* INIT */
	pthread_t splashScreenThread;
	_Bool* splashScreenPlaying = ledMatrix_init(&splashScreenThread);// Enables LED display + displays splash screen
	Song_data_init();								// Obtains list of songs
	Joystick_init();								// Enables joystick inputs
	POT_init();										// Enables potentiometer (volume)
	Audio_init(NUM_CHANNELS, SAMPLE_RATE);			// Enables .mp3 & .wav audio

	// ledMatrix_song_list(song songList[], int nextSong, int colour); 
	// ledMatrix_music_track_display("EXO", 1114197, DEFAULT_ROW_OFFSET);
	// ledMatrix_music_timer(100, 1114197, DEFAULT_HORIZONTAL_OFFSET);
	// ledMatrix_music_timer(Song_data_getTimer(), 16764159, 0);

	// ledMatrix_start_music_timer(true);

	// Stop splash screen
	*splashScreenPlaying = false;
	pthread_join(splashScreenThread, NULL);
	free(splashScreenPlaying);
	// Show menu
	ledMatrix_display_song_list();

	while(true){
		if(isPlaying && !isMenu){
			ledMatrix_music_timer(Song_data_getTimer(), 16764159, 0);
			ledMatrix_refresh();
			ledMatrix_timer_clear();
		}
		else{
			ledMatrix_refresh();
		}
	}

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
