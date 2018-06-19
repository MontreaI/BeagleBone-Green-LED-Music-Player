/*
 *  Small program to read a wave file and play it.
 *  Heavily based on an example by Brian Fraser, which is heavily based on code found at:
 *  http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_min_8c-example.html
 */

#include <stdint.h>

#include <alsa/asoundlib.h>

#include "joystick.h"
#include "wave_player.h"

// File used for play-back:
// If cross-compiling, must have this file available, via this relative path,
// on the target when the application is run. This example's Makefile copies the wave-files/
// folder along with the executable to ensure both are present.
//#define SOURCE_FILE "wave-files/100060__menegass__gui-drum-splash-hard.wav"
#define SOURCE_FILE "wave-files/a2002011001-e02.wav"


int main(void)
{
	printf("Beginning play-back of %s\n", SOURCE_FILE);

	currentVolume = 80;

	stop = false;
	Joystick_startPolling();

	// Load wave file we want to play:
	wavedata_t sampleFile;
	Audio_readWaveFileIntoMemory(SOURCE_FILE, &sampleFile);

	// Configure Output Device
	snd_pcm_t *handle = Audio_openDevice(sampleFile.numChannels, sampleFile.sampleRate);


	// Play Audio
	Audio_playFile(handle, &sampleFile);
//	Audio_playFile(handle, &sampleFile);
//	Audio_playFile(handle, &sampleFile);

	// Cleanup, letting the music in buffer play out (drain), then close and free.
	snd_pcm_drain(handle);
	snd_pcm_hw_free(handle);
	snd_pcm_close(handle);
	free(sampleFile.pData);

	stop = true;
	Joystick_stopPolling();

	printf("Done!\n");
	return 0;
}



// Open the PCM audio output device and configure it.
// Returns a handle to the PCM device; needed for other actions.
snd_pcm_t *Audio_openDevice(unsigned int numChannels, unsigned int sampleRate)
{
	snd_pcm_t *handle;

	// Open the PCM output
	int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Play-back open error: %s\n", snd_strerror(err));
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
		printf("Play-back configuration error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	return handle;
}

// Read in the file to dynamically allocated memory.
// !! Client code must free memory in wavedata_t !!
void Audio_readWaveFileIntoMemory(char *fileName, wavedata_t *pWaveStruct)
{
	assert(pWaveStruct);

	// Wave file has 44 bytes of header data. This code assumes file
	// is correct format.
	const int DATA_OFFSET_INTO_WAVE = 44;

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

// Play the audio file (blocking)
void Audio_playFile(snd_pcm_t *handle, wavedata_t *pWaveData)
{
	// If anything is waiting to be written to screen, can be delayed unless flushed.
	fflush(stdout);

	// Write data and play sound (blocking)
	snd_pcm_sframes_t frames = snd_pcm_writei(handle, pWaveData->pData, pWaveData->numSamples);

	// Check for errors
	if (frames < 0)
		frames = snd_pcm_recover(handle, frames, 0);
	if (frames < 0) {
		fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n", frames);
		exit(EXIT_FAILURE);
	}
	if (frames > 0 && frames < pWaveData->numSamples)
		printf("Short write (expected %d, wrote %li)\n", pWaveData->numSamples, frames);
}