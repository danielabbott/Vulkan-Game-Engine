#include <pigeon/audio/audio.h>
#include <AL/al.h>
#include <AL/alc.h>

static ALCdevice * device;
static ALCcontext * context;

ERROR_RETURN_TYPE pigeon_audio_init(void)
{
    if(device) return 0;

    device = alcOpenDevice(NULL);
    ASSERT_1(device);


    context = alcCreateContext(device, NULL);
    if(!context) {
        pigeon_audio_deinit();
        ASSERT_1(false);
    }

    if(!alcMakeContextCurrent(context)) {
        pigeon_audio_deinit(); 
        ASSERT_1(false);
    }

    if(alGetError()) {
        pigeon_audio_deinit(); 
        ASSERT_1(false);
    }

    return 0;
}

void pigeon_audio_set_listener(vec3 position, vec3 look_at, vec3 look_up, vec3 velocity)
{
    float or[6];
    memcpy(or, look_at, 3*4);
    memcpy(&or[3], look_up, 3*4);

    alListener3f(AL_POSITION, position[0], position[1], position[2]);
    alListener3f(AL_VELOCITY, velocity[0], velocity[1], velocity[2]);
    alListenerfv(AL_ORIENTATION, or);

    alGetError();
}


ERROR_RETURN_TYPE pigeon_audio_create_buffers(unsigned int n, PigeonAudioBuffer* out)
{
    alGenBuffers((ALsizei)n, out);
    ASSERT_1(!alGetError());
    return 0;
}

ERROR_RETURN_TYPE pigeon_audio_upload(PigeonAudioBuffer buffer, PigeonAudioMeta meta,
    void * data, unsigned int length_in_bytes)
{
    ALenum format = meta.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    alBufferData(buffer, format, data, (ALsizei)length_in_bytes, (ALsizei)meta.sample_rate);
    ASSERT_1(!alGetError());
    return 0;
}

void pigeon_audio_destroy_buffers(unsigned int n, PigeonAudioBuffer const* buffers)
{
    alDeleteBuffers((ALsizei)n, buffers);
    alGetError();
}

ERROR_RETURN_TYPE pigeon_audio_create_sources(unsigned int n, PigeonAudioSource* out)
{
    alGenSources((ALsizei)n, out);
    ASSERT_1(!alGetError());
    return 0;
}

void pigeon_audio_destroy_sources(unsigned int n, PigeonAudioSource const* sources)
{
    alDeleteSources((ALsizei)n, sources);
    alGetError();
}

void pigeon_audio_update_source(PigeonAudioSource source, PigeonAudioSourceParameters const* parameters)
{
    alSourcef(source, AL_PITCH, parameters->pitch);
    alSourcef(source, AL_GAIN, parameters->gain);
    alSource3f(source, AL_POSITION, parameters->position[0], parameters->position[1], parameters->position[2]);
    alSource3f(source, AL_VELOCITY, parameters->velocity[0], parameters->velocity[1], parameters->velocity[2]);
    alSourcei(source, AL_LOOPING, parameters->loop);
    alGetError();
}

void pigeon_audio_play(PigeonAudioSource source, PigeonAudioBuffer buffer)
{
    alSourcei(source, AL_BUFFER, (int)buffer);
    alSourcePlay(source);

    alGetError();
}


void pigeon_audio_deinit(void)
{
    if(context) {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        context = NULL;
    }

    if(device) {
        alcCloseDevice(device);
        device = NULL;
    }
}
