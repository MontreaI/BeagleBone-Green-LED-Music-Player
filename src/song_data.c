#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

#include "audio.h"

#define SONG_DIR "wave-files/"	// Song directory

char** songBuffer;				// Stores list of songs
static int songBufferSize = 0; 		// Stores num of songs
static int currentSong = 0;		// Current Song playing
wavedata_t currentSongFile;

// Function Prototypes
static char** getFilenames(char *dirName, int* pFilenameCount);

// INIT
void Song_data_init(){
	printf("Song_data_init()\n");

	// Obtain list of Songs
	songBuffer = getFilenames(SONG_DIR, &songBufferSize);
	
	// for(size_t i = 0; i < songBufferSize; i++)
	// {
	// 	printf("%s\n", songBuffer[i]);
	// }
	
}

// Plays song at index
void Song_data_playSong(int index){
	currentSong = index;
	Audio_readWaveFileIntoMemory(songBuffer[index], &currentSongFile);
	Audio_queueSound(&currentSongFile);
}

// Play Previous Song
void Song_data_playPrev(){
	if(currentSong <= 0){
		currentSong = songBufferSize - 1;
	}
	else{
		currentSong--;
	}

	printf("Playing: %s\n", songBuffer[currentSong]);
	Audio_freeWaveFileData(&currentSongFile);
	Audio_readWaveFileIntoMemory(songBuffer[currentSong], &currentSongFile);
	Audio_queueSound(&currentSongFile);
}

// Replay Current Song
void Song_data_replay(){
	printf("Playing: %s\n", songBuffer[currentSong]);

	Audio_freeWaveFileData(&currentSongFile);
	Audio_readWaveFileIntoMemory(songBuffer[currentSong], &currentSongFile);
	Audio_queueSound(&currentSongFile);
}

// Play Next Song
void Song_data_playNext(){
	if(currentSong >= (songBufferSize - 1)){
		currentSong = 0;
	}
	else{
		currentSong ++;
	}

	printf("Playing: %s\n", songBuffer[currentSong]);
	Audio_freeWaveFileData(&currentSongFile);
	Audio_readWaveFileIntoMemory(songBuffer[currentSong], &currentSongFile);
	Audio_queueSound(&currentSongFile);
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
		*pFilenameCount = count;
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
					
					if (strcmp(&songName[strlen(songName)-3], "wav") == 0) {
						strcpy(dest[i], songName);
						i++;
					}			
				}
			}
			currEntity = readdir(pDir);
		}
		closedir(pDir);
	}
	return dest;
}
