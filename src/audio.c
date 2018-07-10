
#include <alsa/asoundlib.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h> // memset, strlen, strcpy
#include <pthread.h>
#include <limits.h>
#include <alloca.h> // needed for mixer
#include <time.h>

#include "audio.h"
#include "joystick.h"
#include "pot.h"
#include "led_matrix.h"

// Private functions forward declarations
static char** getFilenames(char *dirName, int* pFilenameCount);
static void fillPlaybackBuffer(short *playbackBuffer, int size);
static void* playbackThread(void* arg);

static snd_pcm_t *handle;

#define DEFAULT_VOLUME 80

#define SAMPLE_SIZE (sizeof(short)) 			// bytes per sample
// Sample size note: This works for mono files because each sample ("frame') is 1 value.
// If using stereo files then a frame would be two samples.

static unsigned long periodSize = 0; //number of frames per buffer
static unsigned long playbackBufferSize = 0;
static short *playbackBuffer = NULL;
static long long loopCount = 0;

// Currently active (waiting to be played) sound bites
#define MAX_SOUND_BITES 1
typedef struct {
	// A pointer to a previously allocated sound bite (wavedata_t struct).
	// Note that many different sound-bite slots could share the same pointer
	// (overlapping cymbal crashes, for example)
	wavedata_t *pSound;

	// The offset into the pData of pSound. Indicates how much of the
	// sound has already been played (and hence where to start playing next).
	int location;
} playbackSound_t;
static playbackSound_t soundBites[MAX_SOUND_BITES];
static char** filenames = NULL;;
static int filenameCount = 0;

// Playback threading
void* playbackThread(void* arg);
static _Bool stop = false;
static _Bool paused = false;
static pthread_t playbackThreadId;
static pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pauseCond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t stopCond = PTHREAD_COND_INITIALIZER;

static int volume = 0;

// File used for play-back:
// If cross-compiling, must have this file available, via this relative path,
// on the target when the application is run. This example's Makefile copies the wave-files/
// folder along with the executable to ensure both are present.
//#define SOURCE_FILE "wave-files/100060__menegass__gui-drum-splash-hard.wav"
#define SOURCE_FILE "wave-files/a2002011001-e02.wav"
#define WAVE_FILE_DIR "wave-files/"

int main(void)
{
	printf("Beginning play-back of %s\n", SOURCE_FILE);// Load wave file we want to play:
	wavedata_t sampleFile;
	//Audio_readWaveFileIntoMemory(SOURCE_FILE, &sampleFile);

	// Get filenames
	filenames = getFilenames(WAVE_FILE_DIR, &filenameCount);
	char filenameBuffer[100];
	strcpy(filenameBuffer, WAVE_FILE_DIR);
	strcpy(filenameBuffer + strlen(WAVE_FILE_DIR), filenames[11]);

	Audio_readWaveFileIntoMemory(SOURCE_FILE, &sampleFile);
	stop = false;

	// Configure Output Device
	Audio_init(sampleFile.numChannels, sampleFile.sampleRate);
	Joystick_init();
	POT_init();

	// Play Audio
	Audio_queueSound(&sampleFile);

	// Wait until stop
	pthread_mutex_lock(&audioMutex);
	pthread_cond_wait(&stopCond, &audioMutex);
	pthread_mutex_unlock(&audioMutex);


	// Cleanup, letting the music in buffer play out (drain), then close and free.
	Audio_cleanup();
	Joystick_cleanup();

	printf("Done!\n");
	return 0;
}

void Audio_init(unsigned int numChannels, unsigned int sampleRate)
{
	Audio_setVolume(DEFAULT_VOLUME);

	// Initialize the currently active sound-bites being played
	// REVISIT:- Implement this. Hint: set the pSound pointer to NULL for each
	//     sound bite.
	pthread_mutex_lock(&audioMutex);
	for (int i = 0; i < MAX_SOUND_BITES; i++) {
		soundBites[i].pSound = NULL;
	}
	pthread_mutex_unlock(&audioMutex);

	// Open the PCM output
	int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	// Configure parameters of PCM output
	err = snd_pcm_set_params(handle,
			SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			numChannels,
			sampleRate,
			1,			// Allow software resampling
			50000);		// 0.05 seconds per buffer
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	// Allocate this software's playback buffer to be the same size as the
	// the hardware's playback buffers for efficient data transfers.
	// ..get info on the hardware buffers:
 	unsigned long unusedBufferSize = 0;
	snd_pcm_get_params(handle, &unusedBufferSize, &periodSize);
	playbackBufferSize = periodSize * numChannels;
	// ..allocate playback buffer:
	playbackBuffer = malloc(playbackBufferSize * sizeof(*playbackBuffer));

	// Launch playback thread:
	pthread_create(&playbackThreadId, NULL, playbackThread, NULL);
}

// Allocates and returns array containing names of files in dirName
static char** getFilenames(char *dirName, int* pFilenameCount)
{
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
				dest[i] = malloc(strlen(currEntity->d_name) + 1);
				if (dest[i]) {
					strcpy(dest[i], currEntity->d_name);
				}
				i++;
			}
			currEntity = readdir(pDir);
		}
		closedir(pDir);
	}
	return dest;
}

// Read in the file to dynamically allocated memory.
// !! Client code must free memory in wavedata_t !!
void Audio_readWaveFileIntoMemory(char *fileName, wavedata_t *pWaveStruct)
{
	assert(pWaveStruct);

	// Wave file has 44 bytes of header data. This code assumes file
	// is correct format.
	const int DATA_OFFSET_INTO_WAVE = 44;

	const int WAVE_FIELD_OFFSET = 8;
	const int NUM_CHANNELS_OFFSET = 22;
	const int SAMPLE_RATE_OFFSET = 24;
	const int SAMPLE_SIZE_OFFSET = 34;
	const int BITS_PER_BYTE = 8;


	// Open file
	FILE *file = fopen(fileName, "r");
	if (file == NULL) {
		fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
		exit(EXIT_FAILURE);
	}

	// Check if fileName is a wave file
	const int fieldSize = 4;
	char fieldBuff[fieldSize+1];
	const char waveHeader[] = "WAVE";
	fseek(file, WAVE_FIELD_OFFSET, SEEK_SET);
	fread(fieldBuff, fieldSize, sizeof(char), file);
	fieldBuff[fieldSize] = '\0';
	if (strcmp(fieldBuff, waveHeader)) {
		return;
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	int sizeInBytes = ftell(file) - DATA_OFFSET_INTO_WAVE;

	uint16_t numChannels;
	fseek(file, NUM_CHANNELS_OFFSET, SEEK_SET);
	fread(&numChannels, 1, sizeof(uint16_t), file);
	pWaveStruct->numChannels = numChannels;


	uint32_t sampleRate;
	fseek(file, SAMPLE_RATE_OFFSET, SEEK_SET);
	fread(&sampleRate, 1, sizeof(uint32_t), file);
	pWaveStruct->sampleRate = sampleRate;

	// Get sample size (bit depth) in bits
	uint16_t sampleSize;
	fseek(file, SAMPLE_SIZE_OFFSET, SEEK_SET);
	fread(&sampleSize, 1, sizeof(uint16_t), file);

	printf("Number of Channels = %d\n", numChannels);
	printf("Sample rate = %d\n", sampleRate);
	printf("Sample size = %d\n", sampleSize);

	fseek(file, DATA_OFFSET_INTO_WAVE, SEEK_SET);
	pWaveStruct->numSamples = sizeInBytes * BITS_PER_BYTE / sampleSize;

	// Allocate Space
	pWaveStruct->pData = malloc(sizeInBytes);
	if (pWaveStruct->pData == NULL) {
		fprintf(stderr, "ERROR: Unable to allocate %d bytes for file %s.\n",
				sizeInBytes, fileName);
		exit(EXIT_FAILURE);
	}

	// Read data:
	int samplesRead = fread(pWaveStruct->pData, sampleSize / BITS_PER_BYTE, pWaveStruct->numSamples, file);
	if (samplesRead != pWaveStruct->numSamples) {
		fprintf(stderr, "ERROR: Unable to read %d samples from file %s (read %d).\n",
				pWaveStruct->numSamples, fileName, samplesRead);
		exit(EXIT_FAILURE);
	}

	fclose(file);
}

void Audio_freeWaveFileData(wavedata_t *pSound)
{
	pSound->numSamples = 0;
	if (!(pSound->pData)) {
		free(pSound->pData);
		pSound->pData = NULL;
	}
}

void Audio_queueSound(wavedata_t *pSound)
{
	// Ensure we are only being asked to play "good" sounds:
	assert(pSound->numSamples > 0);
	assert(pSound->pData);

	pthread_mutex_lock(&audioMutex);
	int i;
	_Bool slotFound = false;
	for (i = 0; i < MAX_SOUND_BITES; i++) {
		playbackSound_t* pCurrBite = soundBites + i;
		if (pCurrBite->pSound == NULL) {
			pCurrBite->pSound = pSound;
			pCurrBite->location = 0;
			slotFound = true;
			break;
		}
	}
	if (!slotFound) {
		//printf("AudioMixer: Error: was unable to queue sound. No free slot found.\n");
	}
	pthread_mutex_unlock(&audioMutex);
}

void Audio_cleanup(void)
{
	printf("Stopping audio...\n");

	// Stop the PCM generation thread
	stop = true;
	if (paused) {
		Audio_togglePlayback();
	}
	pthread_join(playbackThreadId, NULL);

	// Shutdown the PCM output, allowing any pending sound to play out (drain)
	snd_pcm_drain(handle);
	snd_pcm_close(handle);

	// Free playback buffer
	// (note that any wave files read into wavedata_t records must be freed
	//  in addition to this by calling Audio_freeWaveFileData() on that struct.)
	free(playbackBuffer);
	playbackBuffer = NULL;
	for (int i = 0; i < MAX_SOUND_BITES; i++) {
		Audio_freeWaveFileData(soundBites[MAX_SOUND_BITES].pSound);
	}
	for (int i = 0; i < filenameCount; i++) {
		free(filenames[i]);
	}
	free(filenames);

	printf("Done stopping audio...\n");
	fflush(stdout);
}


int Audio_getVolume()
{
	// Return the cached volume; good enough unless someone is changing
	// the volume through other means and the cached value is out of date.
	return volume;
}

// Function copied from:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
// Written by user "trenki".
void Audio_setVolume(int newVolume)
{
	// Ensure volume is reasonable; If so, cache it for later getVolume() calls.
	if (newVolume < 0 || newVolume > AUDIO_MAX_VOLUME) {
		printf("ERROR: Volume must be between 0 and 100.\n");
		return;
	}
	volume = newVolume;

    long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "PCM";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

    snd_mixer_close(handle);
}

void Audio_togglePlayback() {
	paused = !paused;
	if(!paused) {
		pthread_cond_signal(&pauseCond);
	}
}

// Fill the playbackBuffer array with new PCM values to output.
//    playbackBuffer: buffer to fill with new PCM data from sound bites.
//    size: the number of values to store into playbackBuffer
static void fillPlaybackBuffer(short *playbackBuffer, int size)
{

	memset(playbackBuffer, 0, sizeof(short) * size);
	int* sumHolder = malloc(sizeof(int) * size);
	memset(sumHolder, 0, sizeof(int) * size);
	pthread_mutex_lock(&audioMutex);
	for (int i = 0; i < MAX_SOUND_BITES; i++) {
		playbackSound_t* pCurrBite = soundBites + i;
		if (pCurrBite->pSound != NULL) {
			short* pCurrData = pCurrBite->pSound->pData;
			int sampleCount = pCurrBite->pSound->numSamples;
			int currLocation = pCurrBite->location;

			int copyAmount = size;
			_Bool sampleDone = false;
			if (currLocation + size > sampleCount) {
				copyAmount = sampleCount - currLocation;
				sampleDone = true;
			}
			for (int j = 0; j < copyAmount; j++) {
				sumHolder[j] = sumHolder[j] + (int) pCurrData[currLocation + j];
			}
			pCurrBite->location += size;
			if (sampleDone) {
				pCurrBite->pSound = NULL;
				stop = true;
			}
		}
	}
	pthread_mutex_unlock(&audioMutex);
	for (int i = 0; i < size; i++) {
		if (sumHolder[i] > SHRT_MAX) {
			playbackBuffer[i] = SHRT_MAX;
		} else if (sumHolder[i] < SHRT_MIN) {
			playbackBuffer[i] = SHRT_MIN;
		} else {
			playbackBuffer[i] = sumHolder[i];
		}
	}
	free(sumHolder);
}

static void* playbackThread(void* arg)
{
//	const struct timespec sleepTime = {.tv_nsec = 0, .tv_sec = 15};
//	nanosleep(&sleepTime, NULL); // give time for sound bites to be queued
	while (!stop) {
		if (paused) {
			pthread_mutex_lock(&audioMutex);
			pthread_cond_wait(&pauseCond, &audioMutex);
			pthread_mutex_unlock(&audioMutex);
		}
		// Generate next block of audio
		fillPlaybackBuffer(playbackBuffer, playbackBufferSize);
		loopCount++;
		//printf("loopcount = %lld\n", loopCount);
		// Output the audio
		snd_pcm_sframes_t frames = snd_pcm_writei(handle,
				playbackBuffer, periodSize);

		// Check for (and handle) possible error conditions on output
		if (frames < 0) {
			fprintf(stderr, "AudioMixer: writei() returned %li\n", frames);
			frames = snd_pcm_recover(handle, frames, 1);
		}
		if (frames < 0) {
			fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n",
					frames);
			exit(EXIT_FAILURE);
		}
		if (frames > 0 && frames < periodSize) {
			printf("Short write (expected %li, wrote %li)\n",
					playbackBufferSize, frames);
		}
	}

	return NULL;
}
















