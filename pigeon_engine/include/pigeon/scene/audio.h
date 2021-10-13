#include <pigeon/scene/component.h>
#include <stdbool.h>
#include <pigeon/audio/audio.h>

struct PigeonTransform;

// PIGEON_COMPONENT_TYPE_AUDIO_PLAYER
// N.B. Only attach this to 1 transform, additional transform objects are ignored
typedef struct PigeonAudioPlayer
{  
    PigeonComponent c;
    PigeonAudioSourceID _source_id;

    float gain;
    bool loop;    
} PigeonAudioPlayer;

PigeonAudioPlayer* pigeon_create_audio_player(void);
void pigeon_destroy_audio_player(PigeonAudioPlayer *);

void pigeon_audio_player_play(PigeonAudioPlayer*, PigeonAudioBufferID);

PIGEON_ERR_RET pigeon_update_scene_audio(struct PigeonTransform * camera);
