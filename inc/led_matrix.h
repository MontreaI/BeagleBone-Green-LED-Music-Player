#ifndef LED_MATRIX_H
#define LED_MATRIX_H

void ledMatrix_init();
/*
 * Call it in a loop in external code to change timer, can be used for the following:
 * 1. Call in a loop to change the timer as song progresses
 * 2. If stop calling it the timer will stop changing on previous call and will act as a "pause" 
*/
void ledMatrix_music_timer(int seconds);
/*
 * Displays the track name on the first half of the LED matrix rows
*/
void ledMatrix_track_display(char* track);
/*
 * Grabs the track name (usually the file name...), album (pass NULL if not exist), artist (pass NULL if not exist)
*/
int ledMatrix_music_details(char* track, char* album, char* artist);
/*
 * Volume is from 0 - 100, anything lower or higher will be bounded.
*/
void ledMatrix_music_volume(int volume); //TODO
/*
 * Mode 1: Shuffle Off
 * Mode 2: Shuffle On
 * Mode 3: Repeat On
*/
void ledMatrix_music_mode(int mode); // TODO
/*
 * Convert string to codes used for the LED matrix
*/
int **ledMatrix_extract_string(char* string);

void ledMatrix_clean();

void ledMatrix_clear();
#endif