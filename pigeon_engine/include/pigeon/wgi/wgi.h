#pragma once

#include "window.h"
#include <stdbool.h>
#include <stdint.h>
#include "pipeline.h"
#include "uniform.h"
#include "mesh.h"
#include "shadow.h"
#include "animation.h"
#include "rendergraph.h"
#include "draw.h"


// Returns 0 on success. If fails, call pigeon_wgi_deinit() to cleanup
PIGEON_ERR_RET pigeon_wgi_init(PigeonWindowParameters window_parameters, bool prefer_dedicated_gpu, bool prefer_opengl,
	PigeonWGIRenderConfig render_cfg, float znear, float zfar);





bool pigeon_wgi_multithreading_supported(void);
bool pigeon_wgi_multidraw_supported(void);

// In bytes
// 1 for Vulkan
// 16, 64, 256, etc. for OpenGL
unsigned int pigeon_wgi_get_draw_data_alignment(void);

// Alignment (number of bones) of bone data for 1 armature in the bone uniform data
// This is to align uniform blocks when using OpenGL
// first_bone_offset must be a multiple of this
unsigned int pigeon_wgi_get_bone_data_alignment(void);




void pigeon_wgi_set_depth_range(float znear, float zfar);
void pigeon_wgi_set_bloom_intensity(float i);
void pigeon_wgi_set_brightness(float b);
void pigeon_wgi_set_ambient(float r, float g, float b);
void pigeon_wgi_set_ssao_cutoff(float cb);



// delayed_timer_values is set to the timer query results from 2 or more frames ago
PIGEON_ERR_RET pigeon_wgi_next_frame_wait(double delayed_timer_values[PIGEON_WGI_RENDER_STAGE__COUNT]);
PIGEON_ERR_RET pigeon_wgi_next_frame_poll(double delayed_timer_values[PIGEON_WGI_RENDER_STAGE__COUNT], bool* ready);


// max_draws determines the minimum size of the draws ssbo
// max_multidraw_draws = maximum number of draws within multidraw draws
// Instancing counts as multiple draws
// index into shadows = index into lights array in per-frame uniform data
// draw_objects and bone_matrices are set to point to a uniform data mapping
// use pigeon_wgi_get_draw_data_alignment and pigeon_wgi_get_bone_data_alignment
PIGEON_ERR_RET pigeon_wgi_start_frame(uint32_t max_draws,
    uint32_t max_multidraw_draws,
    PigeonWGIShadowParameters shadows[4], unsigned int total_bones,
    void ** draw_objects,
    PigeonWGIBoneMatrix ** bone_matrices);


// Blocks when using OpenGL
// Call record functions first
PIGEON_ERR_RET pigeon_wgi_submit_frame(void);


void pigeon_wgi_wait_events(void);

// Waits for GPU idle. Use before freeing all resources at application exit
void pigeon_wgi_wait_idle(void);
void pigeon_wgi_deinit(void);

uint64_t pigeon_wgi_get_time_micro(void);
uint32_t pigeon_wgi_get_time_millis(void);
float pigeon_wgi_get_time_seconds(void);
double pigeon_wgi_get_time_seconds_double(void);

// call pigeon_wgi_set_depth_range to set depth range variables
void pigeon_wgi_perspective(mat4 m, float fovy, float aspect);
