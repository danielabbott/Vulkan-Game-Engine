#pragma once

#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <cglm/vec4.h>
#include <stdbool.h>

typedef struct PigeonWGIBone
{
    char* name;
    vec4 head;
    vec4 tail; // not needed for computing animation matrices, only for visualising armature
    int parent_index; // -1 if this is the base bone
} PigeonWGIBone;


// Transform of a bone for a given frame within an animation
typedef struct PigeonWGIBoneData
{
    vec4 rotate;
    vec3 translate;
    float scale;
} PigeonWGIBoneData;

typedef struct PigeonWGIBoneMatrix
{
    float mat3x4[12];
} PigeonWGIBoneMatrix;
