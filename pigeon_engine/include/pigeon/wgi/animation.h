#pragma once

#include "bone.h"
#include <pigeon/util.h>
#include <stdint.h>

struct PigeonAsset;

typedef struct PigeonWGIAnimationMeta {
    const char * name;
    float fps;
    bool loops;
    unsigned int frame_count;    
} PigeonWGIAnimationMeta;
