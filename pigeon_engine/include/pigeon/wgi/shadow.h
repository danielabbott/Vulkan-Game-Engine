#pragma once

#include "image_formats.h"

#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <cglm/mat4.h>

typedef struct PigeonWGIShadowParameters {
    unsigned int resolution; // 1024 is a good default. 0 = shadows disabled
    float near_plane; // must not be > 0
    float far_plane; // must be > near_plane
    float sizeX;
    float sizeY;
    mat4 inv_view_matrix;


    // set by pigeon_wgi_set_frame, do not touch

    int framebuffer_index; 
    mat4 proj_view;
} PigeonWGIShadowParameters;


