#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

#include "audio.h"
#include "stack.h"

#define SONG_DIR "wave-files/"		// Song directory


// SONG BUFFER
static char** songBuffer;			// Song List
static int songBufferSize = 0; 		// Stores num of songs
static int currentSong = 0;			// Current Song playing
static wavedata_t currentSongFile;

// TIMER
static time_t startSongTime;		// Time which song started playing

static _Bool repeat = false;		// Repeat
static _Bool shuffle = false;		// Shuffle

// FUNCTION PROTOTYPES
static char** getFilenames(char *dirName, int* pFilenameCount);

// INIT
void Song_data_init(){
	printf("Song_data_init()\n");

	// Obtain list of Songs
	songBuffer = getFilenames(SONG_DIR, &songBufferSize);

	// Sort songBuffer
	// REVISIT: To use more efficient algorithm
	for (size_t i = 0; i < songBufferSize - 1; i++){
		int minimum = i;
		for (size_t j = i + 1; j < songBufferSize; j++){
			if(strcmp(songBuffer[j], songBuffer[minimum]) < 0){
				minimum = j;
			}
		}

		// Swap (this took me hours trying to do it the other way)
		char *temp = (char*)malloc(strlen(songBuffer[i]) + 1);
		strcpy(temp,songBuffer[i]);
		songBuffer[i] = songBuffer[minimum];
		songBuffer[minimum] = temp;
	}

	printf("songBufferSize: %d\n", songBufferSize);

	// DEBUGGING:
	// for(size_t i = 0; i < songBufferSize; i++)
	// {
	// 	printf("%s\n", songBuffer[i]);
	// }
}

// Starts timer of how long current song is playing
void Song_data_startTimer(){
	time(&startSongTime);
}

// Returns how long current song has been playing in seconds 
int Song_data_getTimer(){
	time_t currentSongTime;

	time(&currentSongTime);

	int seconds = difftime(currentSongTime, startSongTime);

	return seconds;
}

// Return bool of repeat
_Bool Song_data_getRepeat(){
	return repeat;
}

// Return bool of shuffle
_Bool Song_data_getShuffle(){
	return shuffle;
}

// Toggle Repeat
void Song_data_toggleRepeat(){
	repeat = !repeat;
}

// Toggle Shuffle
void Song_data_toggleShuffle(){
	shuffle = !shuffle;
}

// Plays song at index
void Song_data_playSong(int index){
	currentSong = index;
	int len = strlen(songBuffer[currentSong]);

	printf("Playing: %s\n", songBuffer[currentSong]);
	Audio_setPause(true);
	if (strcmp(&songBuffer[currentSong][len-3], "wav") == 0) {
		Audio_freeMusicFileData(&currentSongFile);
		Audio_readWaveFileIntoMemory(songBuffer[currentSong], &currentSongFile);
	}
	else if (strcmp(&songBuffer[currentSong][len-3], "mp3") == 0) {
		Audio_freeMusicFileData(&currentSongFile);
		Audio_readMP3FileIntoMemory(songBuffer[currentSong], &currentSongFile);
	}
	Audio_queueSound(&currentSongFile);
	Audio_setPause(false);
}

// Replay Current Song
void Song_data_replay(){
	printf("Replaying: %s\n", songBuffer[currentSong]);

	Song_data_playSong(currentSong);
}

// Play Previous Song
void Song_data_playPrev(){
	// SHUFFLE ON
	if (shuffle){
		// play previous shuffled song
		int poppedSong = Stack_pop();

		// Empty Stack
		if(poppedSong == -1){
			Song_data_replay();
			return;
		}
		else{
			currentSong = poppedSong;
		}
	}
	// SHUFFLE OFF
	else{
		// free shuffle struct
		Stack_clear();

		if(currentSong <= 0){
			currentSong = songBufferSize - 1;
		}
		else{
			currentSong--;
		}
	}

	printf("Playing Prev %d: %s\n", currentSong, songBuffer[currentSong]);
	Song_data_playSong(currentSong);
}

// Play Next Song
void Song_data_playNext(){

	// SHUFFLE ON
	if (shuffle){
		// generate stack
		Stack_push(currentSong);

		// play random song
		currentSong = rand() % songBufferSize;
	}
	// SHUFFLE OFF
	else{
		// free shuffle list
		Stack_clear();

		if(currentSong >= (songBufferSize - 1)){
			currentSong = 0;
		}
		else{
			currentSong ++;
		}
	}

	printf("Playing Next %d: %s\n", currentSong, songBuffer[currentSong]);
	Song_data_playSong(currentSong);
}

// Allocates and returns array containing names of files in dirName
// Returns number of files in dirName
static char** getFilenames(char *dirName, int* pFilenameCount){
	DIR *pDir;
	struct dirent *currEntity;
	pDir = opendir(dirName);

	char **dest = NULL;
	if (pDir) {
		// count number of regular files
		int count = 0;
		currEntity = readdir(pDir);
		while (currEntity) {
			if (currEntity->d_type == DT_REG) {
				count++;
			}
			currEntity = readdir(pDir);
		}
		dest = malloc(count * sizeof(char*));
		memset(dest, 0, count * sizeof(char*));
		rewinddir(pDir);

		// get filenames of regular files
		int i = 0;
		currEntity = readdir(pDir);
		while (currEntity) {
			if (currEntity->d_type == DT_REG) {
				dest[i] = (char*) malloc(strlen(SONG_DIR) + strlen(currEntity->d_name) + 1);
				if (dest[i]) {
					// Returns format: "wave-files/a2002011001-e02.wav"
					char* songName = (char*) malloc(strlen(SONG_DIR) + strlen(currEntity->d_name) + 1);
					strcpy(songName, SONG_DIR);
					strcat(songName, currEntity->d_name);
					strcpy(dest[i], songName);
					i++;		
				}
			}
			currEntity = readdir(pDir);
		}
		closedir(pDir);

		// Set pFilenameCount to the number of wave files
		*pFilenameCount = i;
	}
	return dest;
}
