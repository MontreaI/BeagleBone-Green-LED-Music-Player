// Methods relating to the metadata of a song

#ifndef SONG_DATA_H
#define SONG_DATA_H

void Song_data_init();
void Song_data_cleanup();

void Song_data_playMP3();
void Song_data_playSong(int index);		// Play song of index
void Song_data_playPrev();				// Plays previous song
void Song_data_playNext();				// Plays next song (Depends if shuffle on/off)
void Song_data_replay();				// Replays current song

void Song_data_startTimer();			// Start current song timer 
										// Called by Audio_queueSound

int Song_data_getTimer();				// Return current playing time in seconds
_Bool Song_data_getRepeat();			// Return true if repeat is on
_Bool Song_data_getShuffle();			// Return true if shuffle is on

void Song_data_toggleRepeat();			// Toggle Repeat
void Song_data_toggleShuffle();			// Toggle Shuffle

char* Song_data_getCurrentSong();

#endif