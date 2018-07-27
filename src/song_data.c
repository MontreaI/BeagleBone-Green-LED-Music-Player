#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <mpg123.h>
#include <math.h>
#include <stdint.h>

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
struct SongData{
	char* songDir;					// Directory to Song 
	char* songName;				
	char* artist;
	int duration;
	_Bool isWav;					// True = wav, False = mp3
};

// Song buffer of SongData objects

// SONG BUFFER
static struct SongData *songBuffer;
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
// static void getSongName(char* songName, int index);

/*****************************************************************************
**                CODE STARTS HERE
*****************************************************************************/
void Song_data_init(){
	printf("Song_data_init()\n");

	// Obtain Song Data
	getMetaData(SONG_DIR, &songBufferSize);

	dirLen = strlen(SONG_DIR);		// Obtain directory length

	// // Sort songBuffer
	for (size_t i = 0; i < songBufferSize - 1; i++){
		int minimum = i;
		for (size_t j = i + 1; j < songBufferSize; j++){
			if(strcmp(songBuffer[j].songDir, songBuffer[minimum].songDir) < 0){
				minimum = j;
			}
		}

		// Swap
		struct SongData temp = songBuffer[i];
		songBuffer[i] = songBuffer[minimum];
		songBuffer[minimum] = temp;
	}

	printf("\nsongBufferSize: %d\n", songBufferSize);

	// DEBUGGING:
	// for(size_t i = 0; i < songBufferSize; i++)
	// {
	// 	printf("%s\n", songBuffer[i].songDir);
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

	int len = strlen(songBuffer[currentSong].songDir);

	printf("Playing: %s\n", songBuffer[currentSong].songDir);

	/* Updating the ledDisplay for Song Name */
	// char* songName = (char*) malloc(len - dirLen - 3);
	// getSongName(songName, currentSong);
	Audio_threadInput* pInput = malloc(sizeof(Audio_threadInput));
	pInput->filename = songBuffer[currentSong].songDir;
	pInput->pStop = malloc(sizeof(_Bool));
	if (strcmp(&songBuffer[currentSong].songDir[len-3], "wav") == 0) {
		if (pthread_create(pThreadId, NULL, Audio_playWAV, pInput))
        	printf("ERROR cannot create a new audio playback thread");
	}
	if (strcmp(&songBuffer[currentSong].songDir[len-3], "mp3") == 0) {
		if (pthread_create(pThreadId, NULL, Audio_playMP3, pInput))
        	printf("ERROR cannot create a new audio playback thread");
	}
	return pInput->pStop;
}

// Replay Current Song
_Bool* Song_data_replay(pthread_t* pThreadId){
	printf("Replaying: %s\n", songBuffer[currentSong].songDir);
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

	// printf("Playing Prev %d: %s\n", currentSong, songBuffer[currentSong].songDir);
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

	// printf("Playing Next %d: %s\n", currentSong, songBuffer[currentSong].songDir);
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

		songBuffer = malloc(count * sizeof(struct SongData));
		memset(songBuffer, 0, count * sizeof(struct SongData));

		rewinddir(pDir);
	}
	// Get Metadata within directory
	if (pDir) {
		int i = 0;
		currEntity = readdir(pDir);
		while (currEntity) {
			if (currEntity->d_type == DT_REG) {
				songBuffer[i].songDir = (char*) malloc(strlen(SONG_DIR) + strlen(currEntity->d_name) + 1);
				
				/* Get SongDir */
				// Returns format: "wave-files/a2002011001-e02.wav"
				char* songDir = (char*) malloc(strlen(SONG_DIR) + strlen(currEntity->d_name) + 1);
				strcpy(songDir, SONG_DIR);
				strcat(songDir, currEntity->d_name);
				strcpy(songBuffer[i].songDir, songDir);

				// TODO: Refactor BELOW into functions to gather metadata for mp3 & wav
				int len = strlen(songBuffer[i].songDir);

				/* Get MP3 Metadata */
				// if (strcmp(&songBuffer[i].songDir[len-3], "mp3") == 0) {

				// 	/* mpg123 setup */
				//   	mpg123_init();
				//     mpg123_handle *mh = mpg123_new(NULL, NULL);
				//     mpg123_open(mh, songBuffer[i].songDir);
				// 	mpg123_scan(mh);

				// 	mpg123_id3v1 *v1;
				// 	mpg123_id3v2 *v2;
				// 	/* mpg123 setup */

				// 	// isWav
				// 	songBuffer[i].isWav = false;

				// 	// Get Song Duration
				// 	off_t samples = mpg123_length(mh);			// Get total samples
				// 	int frameDuration = mpg123_spf(mh);			// Get # samples in 1 frame
				// 	double seconds = mpg123_tpf(mh);			// Get # seconds in 1 frame
				// 	int songDuration = ceil((samples / frameDuration) * seconds);
				// 	songBuffer[i].duration = songDuration;

				// 	// Get Song Name + Artist
				// 	char* title = NULL;
				// 	char* artist = NULL;
				// 	if(mpg123_meta_check(mh) & MPG123_ID3 && mpg123_id3(mh, &v1, &v2) == MPG123_OK){
				// 		if(v2 != NULL){
				// 			title = v2->title->p;
				// 			songBuffer[i].songName = title;

				// 			artist = v2->artist->p;
				// 			songBuffer[i].artist = artist;
				// 		}
				// 		else if(v1 != NULL) {
				// 			songBuffer[i].songName = v1->title;
				// 			songBuffer[i].artist = v1->artist;
				// 		}
				// 	}
				// 	mpg123_delete(mh);
				// 	mpg123_exit();
				// }

				/* Get WAV Metadata */
				if (strcmp(&songBuffer[i].songDir[len-3], "wav") == 0){
					// isWav
					songBuffer[i].isWav = true;

					// Get Song Duration
					{
						const int NUM_CHANNELS_OFFSET = 22;
						const int SAMPLE_RATE_OFFSET = 24;
						const int SAMPLE_SIZE_OFFSET = 34;

						FILE *file = fopen(songBuffer[i].songDir, "r");
						if (file == NULL) {
							fprintf(stderr, "Audio: ERROR: Unable to open file %s.\n", songBuffer[i].songDir);
							exit(EXIT_FAILURE);
						}

						fseek(file, 0, SEEK_END);
						int file_size = ftell(file);

						uint16_t numChannels;
						fseek(file, NUM_CHANNELS_OFFSET, SEEK_SET);
						fread(&numChannels, 1, sizeof(uint16_t), file);

						uint32_t sampleRate;
						fseek(file, SAMPLE_RATE_OFFSET, SEEK_SET);
						fread(&sampleRate, 1, sizeof(uint32_t), file);

						// Get sample size (bit depth) in bits
						uint16_t sampleSize;
						fseek(file, SAMPLE_SIZE_OFFSET, SEEK_SET);
						fread(&sampleSize, 1, sizeof(uint16_t), file);

						// FORMULA = (File Size) / (Sample Rate) / (Sample Size) / (Number Channels) * 8
						int total = ceil((float)file_size / (float)sampleRate / (float)sampleSize / (float)numChannels * 8);
						songBuffer[i].duration = total;
					}

					// Get Artist
					char* token = strtok(currEntity->d_name, "-");
					songBuffer[i].artist = token;

					// Get Song Name
					token = strtok(NULL, "-");
					songBuffer[i].songName = token + 1;
				}

				i++;		
			}
			currEntity = readdir(pDir);
		}
		closedir(pDir);

		// Set pFilenameCount to the number of wave files
		*pFilenameCount = i;
	}
}
