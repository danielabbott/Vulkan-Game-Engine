#pragma once

#include <pigeon/util.h>

struct PigeonTransform;
struct PigeonWGIPipeline;

PIGEON_ERR_RET pigeon_draw_frame(struct PigeonTransform * camera, bool debug_disable_ssao, 
    struct PigeonWGIPipeline* skybox_pipeline);
