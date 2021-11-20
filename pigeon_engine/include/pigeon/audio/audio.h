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

typedef unsigned int PigeonAudioBufferID;
typedef unsigned int PigeonAudioSourceID;

PIGEON_ERR_RET pigeon_audio_init(void);

void pigeon_audio_set_listener(vec3 position, vec3 look_at, vec3 look_up, vec3 velocity);

PIGEON_ERR_RET pigeon_audio_create_buffers(unsigned int n, PigeonAudioBufferID* out);
PIGEON_ERR_RET pigeon_audio_upload(PigeonAudioBufferID, PigeonAudioMeta, void*, unsigned int length_in_bytes);

PIGEON_ERR_RET pigeon_audio_create_sources(unsigned int n, PigeonAudioSourceID* out);

void pigeon_audio_update_source(PigeonAudioSourceID, PigeonAudioSourceParameters const*);
void pigeon_audio_play(PigeonAudioSourceID, PigeonAudioBufferID);

void pigeon_audio_destroy_sources(unsigned int n, PigeonAudioSourceID const*);
void pigeon_audio_destroy_buffers(unsigned int n, PigeonAudioBufferID const*);

void pigeon_audio_deinit(void);
