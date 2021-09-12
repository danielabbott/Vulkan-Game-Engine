#pragma once

#include <cglm/vec3.h>
#include <pigeon/util.h>

typedef struct PigeonAudioMeta {
    unsigned int sample_rate;
    unsigned int channels;
} PigeonAudioMeta;

typedef struct PigeonAudioSourceParameters {
    float pitch; // default 1
    float gain; // default 1
    vec3 position;
    vec3 velocity;
    bool loop;
} PigeonAudioSourceParameters;

typedef unsigned int PigeonAudioBuffer;
typedef unsigned int PigeonAudioSource;

ERROR_RETURN_TYPE pigeon_audio_init(void);

void pigeon_audio_set_listener(vec3 position, vec3 look_at, vec3 look_up, vec3 velocity);

ERROR_RETURN_TYPE pigeon_audio_create_buffers(unsigned int n, PigeonAudioBuffer* out);
ERROR_RETURN_TYPE pigeon_audio_upload(PigeonAudioBuffer, PigeonAudioMeta, void *, unsigned int length_in_bytes);


ERROR_RETURN_TYPE pigeon_audio_create_sources(unsigned int n, PigeonAudioSource* out);

void pigeon_audio_update_source(PigeonAudioSource, PigeonAudioSourceParameters const*);
void pigeon_audio_play(PigeonAudioSource, PigeonAudioBuffer);

void pigeon_audio_destroy_sources(unsigned int n, PigeonAudioSource const*);
void pigeon_audio_destroy_buffers(unsigned int n, PigeonAudioBuffer const*);

void pigeon_audio_deinit(void);
