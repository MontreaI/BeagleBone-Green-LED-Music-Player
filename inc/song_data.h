// Methods relating to the metadata of a song

#ifndef SONG_DATA_H
#define SONG_DATA_H

extern char** songBuffer;				// Array of songs
extern wavedata_t currentSongFile;		// File of current song being played

void Song_data_init();
void Song_data_cleanup();

void Song_data_playSong(int index);		// Play song of index
void Song_data_playPrev();				// Plays previous song
void Song_data_playNext();				// Plays next song
void Song_data_replay();				// Replays current song

void Song_data_startTimer();			// Start current song timer 
										// Called by Audio_queueSound

int Song_data_getTimer();				// Get current playing time in seconds

#endif