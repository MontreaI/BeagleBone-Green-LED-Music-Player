#include <alsa/asoundlib.h>
#include <alloca.h>

#include "sound.h"

#define MAX_VOLUME 100

// from audioMixer_template.c
void Sound_setVolume(int volume)
{
    if (volume < 0 || volume > MAX_VOLUME)
        exit(1);

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

void Sound_setTempo(int bpm)
{
    snd_seq_queue_tempo_t* tempo;
    snd_seq_queue_tempo_alloca(&tempo);
    snd_seq_queue_tempo_set_tempo(tempo, 60000000 / bpm);
}