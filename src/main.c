#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#include "audio.h"
#include "joystick.h"
#include "pot.h"
#include "song_data.h"
#include "led_matrix.h"
#include "menu.h"

/******************************************************************************
 **              INTERNAL MACRO DEFINITIONS
 ******************************************************************************/
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_NUM_CHANNELS 2

#define DEFAULT_TIMER_COLOR 7
#define DEFAULT_ROW_OFFSET 0

/******************************************************************************
 **              MAIN 
 ******************************************************************************/
int main(void)
{
	/* INIT */
	pthread_t splashScreenThread;
	_Bool* splashScreenPlaying = ledMatrix_init(&splashScreenThread);	// Enables LED display + displays splash screen
	Song_data_init();													// Obtains list of songs
	Joystick_init();													// Enables joystick inputs
	POT_init();															// Enables potentiometer (volume)
	Audio_init(DEFAULT_NUM_CHANNELS, DEFAULT_SAMPLE_RATE);				// Enables .mp3 & .wav audio

	// Stop splash screen
	*splashScreenPlaying = false;
	pthread_join(splashScreenThread, NULL);
	free(splashScreenPlaying);

	// Show menu
	ledMatrix_display_song_list();

	// Main Loop
	while(true){
		if(Song_isPlaying() && !Menu_isMenu()){
			ledMatrix_timer_clear();
			ledMatrix_music_timer(Song_data_getTimer(), DEFAULT_TIMER_COLOR, DEFAULT_ROW_OFFSET);
			ledMatrix_refresh();
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
