#include <pigeon/scene/audio.h>
#include <pigeon/scene/transform.h>
#include <pigeon/object_pool.h>
#include <pigeon/assert.h>
#include <pigeon/misc.h>
#include <cglm/mat4.h>

static PigeonObjectPool pool;

void pigeon_init_audio_player_pool(void);
void pigeon_init_audio_player_pool(void)
{
    pigeon_create_object_pool(&pool, sizeof(PigeonAudioPlayer), true);
}


void pigeon_deinit_audio_player_pool(void);
void pigeon_deinit_audio_player_pool(void)
{
    pigeon_destroy_object_pool(&pool);
}

PigeonAudioPlayer* pigeon_create_audio_player()
{
    PigeonAudioPlayer * ap = pigeon_object_pool_allocate(&pool);
    ASSERT_R0(ap);
    ap->c.type = PIGEON_COMPONENT_TYPE_AUDIO_PLAYER;
    ap->gain = 1;

    
    if(pigeon_audio_create_sources(1, &ap->_source_id)) {
        pigeon_object_pool_free(&pool, ap);
        return NULL;
    }

    return ap;
}

void pigeon_destroy_audio_player(PigeonAudioPlayer * ap)
{
    if(!ap) {
        assert(false);
        return;
    }
	if(ap->_source_id) pigeon_audio_destroy_sources(1, &ap->_source_id);
    pigeon_object_pool_free(&pool, ap);
}

void pigeon_audio_player_play(PigeonAudioPlayer* ap, PigeonAudioBufferID buffer)
{
    assert(ap);
    pigeon_audio_play(ap->_source_id, buffer);
}

static void do_update(void* ap_, void * arg0)
{
    (void)arg0;
    PigeonAudioPlayer * ap = ap_;
    if(!ap->c.transforms || !ap->c.transforms->size) return;

    PigeonTransform * t = *(PigeonTransform **)(ap->c.transforms->elements);
    pigeon_scene_calculate_world_matrix(t);

    PigeonAudioSourceParameters p = {0};
    p.gain = ap->gain;
    p.loop = ap->loop;
    p.pitch = 1;

    vec3 pos = {0};
    glm_mat4_mulv3(t->world_transform_cache, pos, 1, p.position);

    // TODO velocity

    pigeon_audio_update_source(ap->_source_id, &p);
}

PIGEON_ERR_RET pigeon_update_scene_audio(PigeonTransform * camera)
{
    if(!camera) {
        return 0;
    }

    pigeon_scene_calculate_world_matrix(camera);

    vec3 pos = {0};
    glm_mat4_mulv3(camera->world_transform_cache, pos, 1, pos);

    vec3 at = {0, 0, -1};
    glm_mat4_mulv3(camera->world_transform_cache, at, 0, at);

    vec3 up = {0, 1, 0};
    glm_mat4_mulv3(camera->world_transform_cache, up, 0, up);

    vec3 vel = {0}; // TODO

    pigeon_audio_set_listener(pos, at, up, vel);

    pigeon_object_pool_for_each(&pool, do_update, NULL);
    return 0;
}
