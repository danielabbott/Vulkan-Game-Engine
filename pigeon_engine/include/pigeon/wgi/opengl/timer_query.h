#pragma once

#include <stdint.h>
#include <pigeon/util.h>

typedef struct PigeonOpenGLTimerQueryGroup
{
    unsigned int n;
    uint32_t* ids;
} PigeonOpenGLTimerQueryGroup;

PIGEON_ERR_RET pigeon_opengl_create_timer_query_group(PigeonOpenGLTimerQueryGroup*, unsigned int n);

void pigeon_opengl_set_timer_query_value(PigeonOpenGLTimerQueryGroup*, unsigned int i);

void pigeon_opengl_get_timer_query_results(PigeonOpenGLTimerQueryGroup*, double * times);

void pigeon_opengl_destroy_timer_query_group(PigeonOpenGLTimerQueryGroup*);
