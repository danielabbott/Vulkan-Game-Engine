#pragma once

#include "bone.h"
#include <stdint.h>

struct PigeonAsset;

typedef struct PigeonWGIAnimationMeta {
    char * name;
    float fps;
    bool loops;
    unsigned int frame_count;    
} PigeonWGIAnimationMeta;
