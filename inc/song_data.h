// Methods relating to the metadata of a song

#ifndef SONG_DATA_H
#define SONG_DATA_H

void Song_data_init();
void Song_data_cleanup();
void Song_data_exitMenuDisplay();

// These functions start a thread that plays a song,
// and outputs a _Bool* and pthread_t*
// To make the thread terminate, the _Bool should be set to false
_Bool Song_isPlaying();											// Returns isPlaying
_Bool* Song_data_playSong(int index, pthread_t* pThreadId);		// Play song of index
_Bool* Song_data_playPrev(pthread_t* pThreadId);				// Plays previous song
_Bool* Song_data_playNext(pthread_t* pThreadId);				// Plays next song (Depends if shuffle on/off)
_Bool* Song_data_replay(pthread_t* pThreadId);					// Replays current song

void Song_data_startTimer();				// Start current song timer 
int Song_data_getTimer();					// Return current playing time in seconds
void Song_data_pauseTimer();				// Stop pause timer
void Song_data_unpauseTimer();				// Start pause timer

_Bool Song_data_getRepeat();				// Return true if repeat is on
_Bool Song_data_getShuffle();				// Return true if shuffle is on

void Song_data_toggleRepeat();				// Toggle Repeat
void Song_data_toggleShuffle();				// Toggle Shuffle

int Song_data_getSongBufferSize();			// Return int of songBufferSize
char* Song_data_getSongName(int index);		// Return song name index index 
char* Song_data_getArtist(int index);		// Return artst at index
char* Song_data_getAlbum(int index);		// Return album at index
char* Song_data_getYear(int index);			// Return year at index

#endif
