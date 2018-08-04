#include <alsa/asoundlib.h>
#include <mpg123.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h> // memset, strlen, strcpy
#include <pthread.h>
#include <limits.h>
#include <alloca.h> // needed for mixer
#include <time.h>
#include <math.h>

#include "audio.h"
#include "joystick.h"
#include "pot.h"
#include "led_matrix.h"
#include "song_data.h"

/******************************************************************************
 **              INTERNAL MACRO DEFINITIONS
 ******************************************************************************/
#define DEFAULT_VOLUME 		(80)
#define BITS_PER_BYTE 		(8)
#define SECONDS_FOR_REPLAY 	(2)

/******************************************************************************
 **              INTERNAL VARIABLES
 ******************************************************************************/
static snd_pcm_t *handle;

// Static function
static void* audioThread(void *ptr);

// Playback threading
static _Bool stop = false;
static _Bool paused = false;
static pthread_mutex_t threadMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pauseCond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t audioMainCond = PTHREAD_COND_INITIALIZER;
static pthread_t audioThreadId;

static int volume = 0;

static int joystickInputFlag = false;
static Joystick_Input joystickInput;

/******************************************************************************
 **              GLOBAL VARIABLES
 ******************************************************************************/
pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t stopCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pcmHandleMutex = PTHREAD_MUTEX_INITIALIZER;

/******************************************************************************
 **              FUNCTION DEFINITIONS
 ******************************************************************************/
void Audio_init(unsigned int numChannels, unsigned int sampleRate)
{
	printf("Audio_init()\n");
	Audio_setVolume(DEFAULT_VOLUME);

	pthread_mutex_lock(&pcmHandleMutex);
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
	pthread_mutex_unlock(&pcmHandleMutex);
	if (pthread_create(&audioThreadId, NULL, audioThread, NULL))
        printf("ERROR cannot create a new Audio thread");
}

void Audio_cleanup(void)
{
	printf("Stopping audio...\n");

	// Stop the PCM generation thread
	stop = true;
	Audio_setPause(false);

	pthread_mutex_lock(&pcmHandleMutex);
	// Shutdown the PCM output, allowing any pending sound to play out (drain)
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	pthread_mutex_unlock(&pcmHandleMutex);

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

void Audio_setPause(_Bool newVal) {
	paused = newVal;
	pthread_mutex_lock(&pcmHandleMutex);
	snd_pcm_pause(handle, paused); // false = 0 = resume, true = 1 = pause
	pthread_mutex_unlock(&pcmHandleMutex);
	if(!paused) {
		pthread_cond_signal(&pauseCond);
	}
}

_Bool Audio_getPause(void) {
	return paused;
}

void Audio_setJoystickInput(Joystick_Input input) {
	joystickInputFlag = true;
	joystickInput = input;
	pthread_cond_signal(&audioMainCond);
}

void* Audio_playWAV(void* ptr)
{
	Audio_threadInput* pInput = (Audio_threadInput*) ptr;

	// Wave file has 44 bytes of header data. This code assumes file
	// is correct format.
	const int DATA_OFFSET_INTO_WAVE = 44;
	const int WAVE_FIELD_OFFSET = 8;
	const int NUM_CHANNELS_OFFSET = 22;
	const int SAMPLE_RATE_OFFSET = 24;
	const int SAMPLE_SIZE_OFFSET = 34;

	// Open file
	FILE *file = fopen(pInput->filename, "r");
	if (file == NULL) {
		fprintf(stderr, "Audio: ERROR: Unable to open file %s.\n", pInput->filename);
		exit(EXIT_FAILURE);
	}

	fseek(file, 0, SEEK_END);
	int file_size = ftell(file);
	printf("File size: %d\n", file_size);

	// Check if fileName is a wave file
	const int fieldSize = 4;
	char fieldBuff[fieldSize+1];
	const char waveHeader[] = "WAVE";
	fseek(file, WAVE_FIELD_OFFSET, SEEK_SET);
	fread(fieldBuff, fieldSize, sizeof(char), file);
	fieldBuff[fieldSize] = '\0';
	if (strcmp(fieldBuff, waveHeader)) {
		return NULL;
	}

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

	// DEBUGGING:
	// printf("Number of Channels = %d\n", numChannels);
	// printf("Sample rate = %d\n", sampleRate);
	// printf("Sample size = %d\n", sampleSize);
	// FORMULA = (File Size) / (Sample Rate) / (Sample Size) / (Number Channels) * 8
	int total = ceil((float)file_size / (float)sampleRate / (float)sampleSize / (float)numChannels * 8);
	printf("Total Duration: %d\n", total);

	pthread_mutex_lock(&pcmHandleMutex);
	int err = snd_pcm_drop(handle);
	if (err < 0) {
		printf("PCM Drop error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	err = snd_pcm_set_params(handle,
			SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			numChannels,
			sampleRate,
			1,
			1000000);	// 1s
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	err = snd_pcm_prepare(handle);
	if (err < 0) {
		printf("PCM Prepare error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	Song_data_startTimer();
	pthread_mutex_unlock(&pcmHandleMutex);

	// 1 analog sample is represented with 16 bits = 2 bytes
	// 1 frame represents 1 analog sample from all channels
    int frame = numChannels * sampleSize / BITS_PER_BYTE;
	// 2 == the number for multiplying period size == recommended buffer size
	size_t offset = 0;

	unsigned long playbackBufferSize = 0; // size in frames
	unsigned long unusedBufferSize = 0;
	snd_pcm_get_params(handle, &unusedBufferSize, &playbackBufferSize);
	// ..allocate playback buffer:
	size_t buffer_size = playbackBufferSize * frame; // size in bytes
	unsigned char *buffer = malloc(buffer_size);

	fseek(file, DATA_OFFSET_INTO_WAVE, SEEK_SET);
	//unsigned char *buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
	while (!*(pInput->pStop) && fread(buffer, sizeof(unsigned char), buffer_size, file) == buffer_size) {
		snd_pcm_sframes_t frames = snd_pcm_writei(handle, buffer, buffer_size/frame);

		if (frames < 0)
			frames = snd_pcm_recover(handle, frames, 0);
		if (frames < 0) {
			fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n", frames);
			exit(EXIT_FAILURE);
		}

		offset += buffer_size;
		fseek(file, offset, SEEK_SET);
	}
	
	free(buffer);
	fclose(file);

    free(pInput);
	pthread_cond_signal(&audioMainCond);
	return NULL;
}

void* Audio_playMP3(void *ptr)
{
	Audio_threadInput* pInput = (Audio_threadInput*) ptr;

    mpg123_init();
	int mpg123_err;
    mpg123_handle *mh = mpg123_new(NULL, &mpg123_err);

    // open the file and get the decoding format
    mpg123_open(mh, pInput->filename);
	int numChannels, encoding;
	long sampleRate;
    mpg123_getformat(mh, &sampleRate, &numChannels, &encoding);
	size_t frame = numChannels * 16 / BITS_PER_BYTE;
	//unsigned char *buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

	const int MODIFIER = 1000;
	size_t buffer_size = 2 * sampleRate * frame / MODIFIER;
	//unsigned char *buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));


	unsigned long playbackBufferSize = 0;
	unsigned long unusedBufferSize = 0;
	snd_pcm_get_params(handle, &unusedBufferSize, &playbackBufferSize);
	// ..allocate playback buffer:
	unsigned char *buffer = malloc(playbackBufferSize * frame);


	pthread_mutex_lock(&pcmHandleMutex);
	int pcm_err = snd_pcm_drop(handle);
	if (pcm_err < 0) {
		printf("PCM Drop error: %s\n", snd_strerror(pcm_err));
		exit(EXIT_FAILURE);
	}
	pcm_err = snd_pcm_set_params(handle,
			SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			numChannels,
			sampleRate,
			1,
			1000000); // 1s
	if (pcm_err < 0) {
		printf("Playback open error: %s\n", snd_strerror(pcm_err));
		exit(EXIT_FAILURE);
	}
	pcm_err = snd_pcm_prepare(handle);
	if (pcm_err < 0) {
		printf("PCM Prepare error: %s\n", snd_strerror(pcm_err));
		exit(EXIT_FAILURE);
	}
	Song_data_startTimer();
	pthread_mutex_unlock(&pcmHandleMutex);

	size_t done;
	while (!*(pInput->pStop) && mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK) {
		snd_pcm_sframes_t frames = snd_pcm_writei(handle, buffer, done/frame);

		// Check for errors
		if (frames < 0)
			frames = snd_pcm_recover(handle, frames, 0);

		if (frames < 0) {
			fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n", frames);
			exit(EXIT_FAILURE);
		}
	}

	// clean up
    free(buffer);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();

    free(pInput);
	pthread_cond_signal(&audioMainCond);
	return NULL;
}

static void* audioThread(void *ptr) {

	pthread_t threadId;
	
	// _Bool* pStop = Song_data_playSong(0, &threadId);
	_Bool* pStop = malloc(sizeof(_Bool));

	while(true) {
		//sleep
		pthread_mutex_lock(&threadMutex);
		pthread_cond_wait(&audioMainCond ,&threadMutex);
		pthread_mutex_unlock(&threadMutex);

		*pStop = true;
    	pthread_join(threadId, NULL);
		free(pStop);

		// Joystick triggered signal
		if (joystickInputFlag) {
			joystickInputFlag = false;
			switch(joystickInput) {
				case JOYSTICK_RIGHT:
                    pStop = Song_data_playNext(&threadId);
                    break;
                case JOYSTICK_LEFT:
				{
                	int playTime = Song_data_getTimer();

                    if (playTime < SECONDS_FOR_REPLAY) {
                    	pStop = Song_data_playPrev(&threadId);
                    }
                    else{
                    	pStop = Song_data_replay(&threadId);
                    }
                    break;
				}
				case JOYSTICK_IN:
					printf("Thread Playing: %s\n", Song_data_getSongName(ledMatrix_getCurrentSong()));
					pStop = Song_data_playSong(ledMatrix_getCurrentSong(), &threadId);
			}
		}
		// Song ended naturally
		else {
			_Bool replay = Song_data_getRepeat();
			if(replay){
				pStop = Song_data_replay(&threadId);
			}
			else{
				pStop = Song_data_playNext(&threadId);
			}
		}
	}
	return NULL;
}
