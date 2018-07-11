// Methods relating to the metadata of a song

#ifndef SONG_DATA_H
#define SONG_DATA_H

extern char** songBuffer;
extern wavedata_t currentSongFile;

void Song_data_init();
void Song_data_cleanup();

// Play song of index
void Song_data_playSong(int index);

void Song_data_playPrev();		// Plays previous song
void Song_data_playNext();		// Plays next song
void Song_data_replay();		// Replays current song

#endif