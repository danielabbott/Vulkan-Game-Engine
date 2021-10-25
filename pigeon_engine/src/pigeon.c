#include <pigeon/wgi/wgi.h>
#include <pigeon/pigeon.h>
#include <pigeon/assert.h>

void pigeon_init_scene_module(void);
void pigeon_deinit_scene_module(void);
PIGEON_ERR_RET pigeon_init_job_system(unsigned int threads);
void pigeon_deinit_job_system(void);

PIGEON_ERR_RET pigeon_init(void)
{
    ASSERT_R1(!pigeon_init_job_system(pigeon_wgi_multithreading_supported() ? 2 : 1));
    pigeon_init_scene_module();
    return 0;
}

void pigeon_deinit(void)
{
    pigeon_deinit_scene_module();
    pigeon_deinit_job_system();
}