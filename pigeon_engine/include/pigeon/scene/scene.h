#pragma once

#include <pigeon/util.h>

struct PigeonTransform;
struct PigeonWGIPipeline;

PIGEON_ERR_RET pigeon_prepare_draw_frame(struct PigeonTransform * camera);

// Run upload render stage between these

PIGEON_ERR_RET pigeon_draw_frame(bool debug_disable_ssao, struct PigeonWGIPipeline* skybox_pipeline);
