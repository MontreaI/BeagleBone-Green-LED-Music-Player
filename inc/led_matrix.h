#ifndef LED_MATRIX_H
#define LED_MATRIX_H

typedef struct{
    char *track;
    char *album;
    char *artist;
    int duration;

} song;

void ledMatrix_init();

void ledMatrix_splash_screen();

int **ledMatrix_extract_string(char* string);

void ledMatrix_music_timer(int seconds, int colour, int horizontalOffset);

void ledMatrix_music_track_display(char* track, int colour, int rowOffSet);

int ledMatrix_music_details(char* track, int colour, int rowOffSet);

void ledMatrix_music_volume(int volume); //TODO
/*
 * Mode 1: Shuffle Off
 * Mode 2: Shuffle On
 * Mode 3: Repeat On
*/
void ledMatrix_music_mode(int mode); // TODO

void ledMatrix_clear_bottom();

void ledMatrix_clear_top();

void ledMatrix_clean();

void ledMatrix_clear();

// void ledMatrix_song_list(song songList[], int nextSong, int colour);
void ledMatrix_display_song_list();
void ledMatrix_display_next_song();
void ledMatrix_display_prev_song();
void ledMatrix_display_next_info();
void ledMatrix_display_prev_info();
void ledMatrix_display_back();
int ledMatrix_display_info(int index);	// 0 = artist, 1 = album, 2 = year, Return -1 if no info
void ledMatrix_clear_infoIndex();
void ledMatrix_clear_menu();
int ledMatrix_getCurrentSong();

void ledMatrix_refresh(void);

void ledMatrix_start_music_timer(_Bool start);

#endif
