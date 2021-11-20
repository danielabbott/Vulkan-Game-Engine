#include <AL/al.h>
#include <AL/alc.h>
#include <pigeon/assert.h>
#include <pigeon/audio/audio.h>
#include <string.h>

static ALCdevice* device;
static ALCcontext* context;

PIGEON_ERR_RET pigeon_audio_init(void)
{
	if (device)
		return 0;

	device = alcOpenDevice(NULL);
	if (!device) {
		ERRLOG("No audio device found!");
		return 1;
	}

	context = alcCreateContext(device, NULL);
	if (!context) {
		pigeon_audio_deinit();
		ASSERT_R1(false);
	}

	if (!alcMakeContextCurrent(context)) {
		pigeon_audio_deinit();
		ASSERT_R1(false);
	}

	if (alGetError()) {
		pigeon_audio_deinit();
		ASSERT_R1(false);
	}

	return 0;
}

void pigeon_audio_set_listener(vec3 position, vec3 look_at, vec3 look_up, vec3 velocity)
{
	if (!device)
		return;

	float or [6];
	memcpy(or, look_at, 3 * 4);
	memcpy(& or [3], look_up, 3 * 4);

	alListener3f(AL_POSITION, position[0], position[1], position[2]);
	alListener3f(AL_VELOCITY, velocity[0], velocity[1], velocity[2]);
	alListenerfv(AL_ORIENTATION, or);

	alGetError();
}

PIGEON_ERR_RET pigeon_audio_create_buffers(unsigned int n, PigeonAudioBufferID* out)
{
	if (!device)
		return 0;

	alGenBuffers((ALsizei)n, out);
	ASSERT_R1(!alGetError());
	return 0;
}

PIGEON_ERR_RET pigeon_audio_upload(
	PigeonAudioBufferID buffer, PigeonAudioMeta meta, void* data, unsigned int length_in_bytes)
{
	if (!device)
		return 0;

	ALenum format = meta.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	alBufferData(buffer, format, data, (ALsizei)length_in_bytes, (ALsizei)meta.sample_rate);
	ASSERT_R1(!alGetError());
	return 0;
}

void pigeon_audio_destroy_buffers(unsigned int n, PigeonAudioBufferID const* buffers)
{
	if (!device)
		return;

	alDeleteBuffers((ALsizei)n, buffers);
	alGetError();
}

PIGEON_ERR_RET pigeon_audio_create_sources(unsigned int n, PigeonAudioSourceID* out)
{
	if (!device)
		return 0;

	alGenSources((ALsizei)n, out);
	ASSERT_R1(!alGetError());
	return 0;
}

void pigeon_audio_destroy_sources(unsigned int n, PigeonAudioSourceID const* sources)
{
	if (!device)
		return;

	alDeleteSources((ALsizei)n, sources);
	alGetError();
}

void pigeon_audio_update_source(PigeonAudioSourceID source, PigeonAudioSourceParameters const* parameters)
{
	if (!device)
		return;

	alSourcef(source, AL_PITCH, parameters->pitch);
	alSourcef(source, AL_GAIN, parameters->gain);
	alSource3f(source, AL_POSITION, parameters->position[0], parameters->position[1], parameters->position[2]);
	alSource3f(source, AL_VELOCITY, parameters->velocity[0], parameters->velocity[1], parameters->velocity[2]);
	alSourcei(source, AL_LOOPING, parameters->loop);
	alGetError();
}

void pigeon_audio_play(PigeonAudioSourceID source, PigeonAudioBufferID buffer)
{
	if (!device)
		return;

	alSourcei(source, AL_BUFFER, (int)buffer);
	alSourcePlay(source);

	alGetError();
}

void pigeon_audio_deinit(void)
{
	if (!device)
		return;

	if (context) {
		alcMakeContextCurrent(NULL);
		alcDestroyContext(context);
		context = NULL;
	}

	if (device) {
		alcCloseDevice(device);
		device = NULL;
	}
}
