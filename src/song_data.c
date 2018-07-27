#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

#include "audio.h"
#include "stack.h"
#include "led_matrix.h"

/*****************************************************************************
**                MACRO DEFINITIONS
*****************************************************************************/
#define SONG_DIR "wave-files/"		// Song directory

/******************************************************************************
 **              GLOBAL VARIABLES
 ******************************************************************************/
typedef struct {
	char songDir[32];
// 	char songName[32];
// 	char artist[32];
// 	int duration;
} SongData;

// Song buffer of SongData objects

// SONG BUFFER
static char** songBuffer;			// Song List
static int songBufferSize = 0; 		// Stores num of songs
static int currentSong = 0;			// Current Song playing
static int dirLen = 0;				// Length of directory

// TIMER
static time_t startSongTime;		// Time which song started playing

static _Bool repeat = false;		// Repeat
static _Bool shuffle = false;		// Shuffle

/*****************************************************************************
**                FUNCTION PROTO
*****************************************************************************/
static void getMetaData(char *dirName, int* pFilenameCount);
static void getSongName(char* songName, int index);

/*****************************************************************************
**                CODE STARTS HERE
*****************************************************************************/
void Song_data_init(){
	printf("Song_data_init()\n");

	// Obtain list of Songs
	getFilenames(SONG_DIR, &songBufferSize);

	dirLen = strlen(SONG_DIR);		// Obtain directory length

	// Sort songBuffer
	for (size_t i = 0; i < songBufferSize - 1; i++){
		int minimum = i;
		for (size_t j = i + 1; j < songBufferSize; j++){
			if(strcmp(songBuffer[j], songBuffer[minimum]) < 0){
				minimum = j;
			}
		}

		// Swap
		char *temp = (char*)malloc(strlen(songBuffer[i]) + 1);
		strcpy(temp, songBuffer[i]);
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
// Outputs the id of the playback thread at pThreadId (on heap!)
// Returns pointer to stop the current audio playback thread
_Bool* Song_data_playSong(int index, pthread_t* pThreadId){
	currentSong = index;
	int len = strlen(songBuffer[currentSong]);

	printf("Playing: %s\n", songBuffer[currentSong]);

	/* Updating the ledDisplay for Song Name */
	char* songName = (char*) malloc(len - dirLen - 3);
	getSongName(songName, currentSong);

	Audio_threadInput* pInput = malloc(sizeof(Audio_threadInput));
	pInput->filename = songBuffer[currentSong];
	pInput->pStop = malloc(sizeof(_Bool));
	if (strcmp(&songBuffer[currentSong][len-3], "wav") == 0) {
		if (pthread_create(pThreadId, NULL, Audio_playWAV, pInput))
        	printf("ERROR cannot create a new audio playback thread");
	}
	if (strcmp(&songBuffer[currentSong][len-3], "mp3") == 0) {
		if (pthread_create(pThreadId, NULL, Audio_playMP3, pInput))
        	printf("ERROR cannot create a new audio playback thread");
	}
	return pInput->pStop;
}

// Replay Current Song
_Bool* Song_data_replay(pthread_t* pThreadId){
	printf("Replaying: %s\n", songBuffer[currentSong]);
	return Song_data_playSong(currentSong, pThreadId);
}

// Play Previous Song
_Bool* Song_data_playPrev(pthread_t* pThreadId){
	// SHUFFLE ON
	if (shuffle){
		// play previous shuffled song
		int poppedSong = Stack_pop();

		// Empty Stack
		if(poppedSong == -1){
			return Song_data_replay(pThreadId);
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
	return Song_data_playSong(currentSong, pThreadId);
}

// Play Next Song
_Bool* Song_data_playNext(pthread_t* pThreadId){

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

	// printf("Playing Next %d: %s\n", currentSong, songBuffer[currentSong]);
	return Song_data_playSong(currentSong, pThreadId);
}

// Allocates and returns array containing names of files in dirName
// Returns number of files in dirName
static void getMetaData(char *dirName, int* pFilenameCount){
	DIR *pDir;
	struct dirent *currEntity;
	pDir = opendir(dirName);

	// Get count of all files in directory
	if (pDir) {
		int count = 0;
		currEntity = readdir(pDir);
		while (currEntity) {
			if (currEntity->d_type == DT_REG) {
				count++;
			}
			currEntity = readdir(pDir);
		}
		songBuffer = malloc(count * sizeof(char*));
		memset(songBuffer, 0, count * sizeof(char*));
		rewinddir(pDir);
	}
	// Get filenames within directory
	// Returns format: "wave-files/a2002011001-e02.wav"
	if (pDir) {
		int i = 0;
		currEntity = readdir(pDir);
		while (currEntity) {
			if (currEntity->d_type == DT_REG) {
				printf("hello\n");
				songBuffer[i] = (char*) malloc(strlen(SONG_DIR) + strlen(currEntity->d_name) + 1);
				printf("test\n");
				if (songBuffer[i]) {
					printf("world\n");
					// Returns format: "wave-files/a2002011001-e02.wav"
					char* songName = (char*) malloc(strlen(SONG_DIR) + strlen(currEntity->d_name) + 1);
					strcpy(songName, SONG_DIR);
					strcat(songName, currEntity->d_name);
					strcpy(songBuffer[i], songName);
					i++;		
				}
			}
			currEntity = readdir(pDir);
		}
		closedir(pDir);

		// Set pFilenameCount to the number of wave files
		*pFilenameCount = i;
	}
}

// Places song name into songName
// Index is the index of the current song playing
static void getSongName(char* songName, int index){
	int len = strlen(songBuffer[currentSong]);
	strncpy(songName, songBuffer[currentSong] + dirLen, len - dirLen - 4);
	songName[len - dirLen - 4] = 0;
	ledMatrix_music_track_display(songName, 1114197, 0);
}