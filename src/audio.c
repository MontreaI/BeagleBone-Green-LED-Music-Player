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

#include "audio.h"
#include "joystick.h"
#include "pot.h"
#include "led_matrix.h"
#include "song_data.h"

// Private functions forward declarations
static _Bool fillPlaybackBuffer(short *playbackBuffer, int size);
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
pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pauseCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t stopCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t pcmHandleMutex = PTHREAD_MUTEX_INITIALIZER;

static int volume = 0;
static int numChannels_Global = 2;

void Audio_init(unsigned int numChannels, unsigned int sampleRate)
{
	printf("Audio_init()\n");
	Audio_setVolume(DEFAULT_VOLUME);
	numChannels_Global = numChannels;
	// Initialize the currently active sound-bites being played
	// REVISIT:- Implement this. Hint: set the pSound pointer to NULL for each
	//     sound bite.
	pthread_mutex_lock(&audioMutex);
	for (int i = 0; i < MAX_SOUND_BITES; i++) {
		soundBites[i].pSound = NULL;
	}
	pthread_mutex_unlock(&audioMutex);

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

// Read in the file to dynamically allocated memory.
// !! Client code must free memory in wavedata_t !!
void Audio_readWaveFileIntoMemory(char *fileName, wavedata_t *pWaveStruct)
{
	pthread_mutex_lock(&audioMutex);

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

	// DEBUGGING:
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

	// REVISIT: I believe this is causing a seg fault when playing a wav file of channels 2 and then switching to a wav file of channels 1
	// Read data:
	int samplesRead = fread(pWaveStruct->pData, sampleSize / BITS_PER_BYTE, pWaveStruct->numSamples, file);
	if (samplesRead != pWaveStruct->numSamples) {
		fprintf(stderr, "ERROR: Unable to read %d samples from file %s (read %d).\n",
				pWaveStruct->numSamples, fileName, samplesRead);
		exit(EXIT_FAILURE);
	}

	fclose(file);

	pthread_mutex_unlock(&audioMutex);
}

void Audio_freeMusicFileData(wavedata_t *pSound)
{
	pthread_mutex_lock(&audioMutex);
	pSound->numSamples = 0;
	if (!(pSound->pData)) {
		free(pSound->pData);
		pSound->pData = NULL;
	}

	pthread_mutex_unlock(&audioMutex);
}

// Queues pSound into soundBites to be played
// Starts current song timer
void Audio_queueSound(wavedata_t *pSound)
{
	// Ensure we are only being asked to play "good" sounds:
	if(pSound->numSamples <= 0 || !(pSound->pData)) {
		return;
	}

	pthread_mutex_lock(&audioMutex);
	// Queue up song
	soundBites[0].pSound = pSound;
	soundBites[0].location = 0;
	numChannels_Global = pSound->numChannels;
	// Configure parameters of PCM output
	pthread_mutex_lock(&pcmHandleMutex);
	int err = snd_pcm_drop(handle);
	if (err < 0) {
		printf("PCM Drop error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	err = snd_pcm_set_params(handle,
			SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			pSound->numChannels,
			pSound->sampleRate,
			1,			// Allow software resampling
			50000);		// 0.05 seconds per buffer
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	err = snd_pcm_prepare(handle);
	if (err < 0) {
		printf("PCM Prepare error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	pthread_mutex_unlock(&pcmHandleMutex);

	// Start Current Song Timer
	Song_data_startTimer();
	pthread_mutex_unlock(&audioMutex);
}

void Audio_cleanup(void)
{
	printf("Stopping audio...\n");

	// Stop the PCM generation thread
	stop = true;
	Audio_setPause(false);
	pthread_join(playbackThreadId, NULL);

	pthread_mutex_lock(&pcmHandleMutex);
	// Shutdown the PCM output, allowing any pending sound to play out (drain)
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	pthread_mutex_unlock(&pcmHandleMutex);

	// Free playback buffer
	// (note that any wave files read into wavedata_t records must be freed
	//  in addition to this by calling Audio_freeWaveFileData() on that struct.)
	free(playbackBuffer);
	playbackBuffer = NULL;
	for (int i = 0; i < MAX_SOUND_BITES; i++) {
		Audio_freeMusicFileData(soundBites[MAX_SOUND_BITES].pSound);
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

// Fill the playbackBuffer array with new PCM values to output.
//    playbackBuffer: buffer to fill with new PCM data from sound bites.
//    size: the number of values to store into playbackBuffer
// Returns true if it is done playing
static _Bool fillPlaybackBuffer(short *playbackBuffer, int size)
{
	pthread_mutex_lock(&audioMutex);

	// Wipe the playbackBuffer to all 0's to clear any previous PCM data.
	memset(playbackBuffer, 0, sizeof(short) * size);

	_Bool donePlaying = false;

	if ( (soundBites[0].pSound != NULL) && (soundBites[0].pSound->numSamples > 0) &&
			(soundBites[0].location < soundBites[0].pSound->numSamples) ){
		// Looping through playbackBuffer
		for (int j = 0; j < size; j++){

			// Adding PCM Samples
			int offset = playbackBuffer[j];
			offset = soundBites[0].pSound->pData[soundBites[0].location];
			
			// Overflow
			if ( (offset > 0) && (offset > SHRT_MAX) ){
				offset = SHRT_MAX;
			}
			// Underflow
			else if ( (offset < 0) && (offset < SHRT_MIN) ){
				offset = SHRT_MIN;
			}

			playbackBuffer[j] = offset;
			soundBites[0].location++;

			// Free soundBite slot if finished playing
			if (soundBites[0].location >= soundBites[0].pSound->numSamples){
				// printf("Location: %d\n", soundBites[0].location);
				// printf("numSamples: %d\n", soundBites[0].pSound->numSamples);
				soundBites[0].pSound = NULL;
				soundBites[0].location = 0;
				donePlaying = true;
				break;
			}
		}
	}

	pthread_mutex_unlock(&audioMutex);

	return donePlaying;

}

void Audio_readMP3FileIntoMemory(char *fileName, wavedata_t *pWaveStruct)
{
	printf("start MP3 to PCM\n");
    mpg123_handle *mh;
    unsigned char *buffer, *mp3;
    size_t buffer_size, done;
    int err, channels, encoding;
    long rate;

    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
	mp3 = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
	
    // open the file and get the decoding format
    mpg123_open(mh, fileName);
    mpg123_getformat(mh, &rate, &channels, &encoding);
    pWaveStruct->numChannels = channels;
    pWaveStruct->sampleRate = rate;

	// decode and load the bytes to memory
	int offset = 0;
	unsigned char *temp;
	while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK) {
		for (int i = 0; i != done; i++) {
            mp3[i+offset] = buffer[i];
        }
		offset += done;
		temp = (unsigned char*)realloc(mp3, offset+done);
		mp3 = temp;
	}
	pWaveStruct->numSamples = offset / (sizeof(char));
    pWaveStruct->pData = (short*) mp3;

    // clean up
    free(buffer);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
	printf("finish MP3 to PCM\n");
}

static void* playbackThread(void* arg){
//	const struct timespec sleepTime = {.tv_nsec = 0, .tv_sec = 15};
//	nanosleep(&sleepTime, NULL); // give time for sound bites to be queued
	while (!stop) {
		if (paused) {
			pthread_mutex_lock(&audioMutex);
			pthread_cond_wait(&pauseCond, &audioMutex);
			pthread_mutex_unlock(&audioMutex);
		}
		// Generate next block of audio
		_Bool donePlaying = fillPlaybackBuffer(playbackBuffer, periodSize * numChannels_Global);

		if(donePlaying){
			// printf("Song done playing\n");
			_Bool replay = Song_data_getRepeat();

			if(replay){
				Song_data_replay();
			}
			else{
				Song_data_playNext();
			}
		}

		loopCount++;
		//printf("loopcount = %lld\n", loopCount);
		// Output the audio

		pthread_mutex_lock(&pcmHandleMutex);
		snd_pcm_sframes_t frames = snd_pcm_writei(handle,
				playbackBuffer, periodSize);
		pthread_mutex_unlock(&pcmHandleMutex);
		// Check for (and handle) possible error conditions on output
		if (frames < 0) {
			fprintf(stderr, "AudioMixer: writei() returned %li, %s\n", frames, snd_strerror(frames));
			frames = snd_pcm_recover(handle, frames, 1);
		}
		if (frames < 0) {
			fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li, %s\n",
					frames, snd_strerror(frames));ESTRPIPE;
			exit(EXIT_FAILURE);
		}
		if (frames > 0 && frames < periodSize) {
			printf("Short write (expected %li, wrote %li)\n",
					playbackBufferSize, frames);
		}
	}

	return NULL;
}
















