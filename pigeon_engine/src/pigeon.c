#include <pigeon/wgi/wgi.h>
#include <pigeon/pigeon.h>
#include <pigeon/assert.h>

void pigeon_init_transform_pool(void);
void pigeon_deinit_transform_pool(void);
void pigeon_init_mesh_renderer_pool(void);
void pigeon_deinit_mesh_renderer_pool(void);
void pigeon_init_pointer_pool(void);
void pigeon_deinit_pointer_pool(void);
void pigeon_init_scene_module(void);
void pigeon_deinit_scene_module(void);
void pigeon_init_light_array_list(void);
void pigeon_deinit_light_array_list(void);
void pigeon_init_audio_player_pool(void);
void pigeon_deinit_audio_player_pool(void);
PIGEON_ERR_RET pigeon_init_job_system(unsigned int threads);

PIGEON_ERR_RET pigeon_init(void)
{
    pigeon_init_scene_module();
    pigeon_init_pointer_pool();
    pigeon_init_transform_pool();
    pigeon_init_mesh_renderer_pool();
    pigeon_init_light_array_list();
    pigeon_init_audio_player_pool();
    ASSERT_R1(!pigeon_init_job_system(pigeon_wgi_multithreading_supported() ? 2 : 1));
    return 0;
}

void pigeon_deinit(void)
{
    pigeon_deinit_scene_module();
    pigeon_deinit_pointer_pool();
    pigeon_deinit_transform_pool();
    pigeon_deinit_mesh_renderer_pool();
    pigeon_deinit_light_array_list();
    pigeon_deinit_audio_player_pool();
}