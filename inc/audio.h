// Methods relating to audio playback

// Playback sounds in real time, allowing multiple simultaneous wave files
// to be mixed together and played without jitter.
#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

typedef struct {
	unsigned int sampleRate;
	unsigned int numChannels;
	unsigned int numSamples;
	short *pData;
} wavedata_t;

#define AUDIO_MAX_VOLUME 100

extern pthread_mutex_t audioMutex;
extern pthread_cond_t stopCond;

// init() must be called before any other functions,
// cleanup() must be called last to stop playback threads and free memory.
void Audio_init(unsigned int numChannels, unsigned int sampleRate);
void Audio_cleanup(void);

// Read the contents of a wave file into the pSound structure. Note that
// the pData pointer in this structure will be dynamically allocated in
// readWaveFileIntoMemory(), and is freed by calling freeWaveFileData().
void Audio_readWaveFileIntoMemory(char *fileName, wavedata_t *pSound);
void Audio_readMP3FileIntoMemory(char *fileName, wavedata_t *pSound);
void Audio_freeMusicFileData(wavedata_t *pSound);

// Queue up another sound bite to play as soon as possible.
void Audio_queueSound(wavedata_t *pSound);

// Get/set the volume.
// setVolume() function posted by StackOverflow user "trenki" at:
// http://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code
int  Audio_getVolume();
void Audio_setVolume(int newVolume);

// Pause/start audio playback.
void Audio_setPause(_Bool newVal);
_Bool Audio_getPause(void);

void MP3_to_PCM(char *fileName, wavedata_t *pWaveStruct);

#endif
