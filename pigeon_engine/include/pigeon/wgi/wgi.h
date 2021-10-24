#pragma once

#include "window.h"
#include <stdbool.h>
#include <stdint.h>
#include "pipeline.h"
#include "uniform.h"
#include "mesh.h"
#include "shadow.h"
#include "animation.h"
#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
    #define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <cglm/types.h>
#include "rendergraph.h"


/* Returns 0 on success. If fails, call pigeon_wgi_deinit() to cleanup */
PIGEON_ERR_RET pigeon_wgi_init(PigeonWindowParameters window_parameters, bool prefer_dedicated_gpu, bool prefer_opengl,
	PigeonWGIRenderConfig render_cfg, float znear, float zfar);

void pigeon_wgi_set_depth_range(float znear, float zfar);
void pigeon_wgi_set_bloom_intensity(float i);

bool pigeon_wgi_close_requested(void);

typedef enum {
    PIGEON_WGI_TIMER_START,
    PIGEON_WGI_TIMER_UPLOAD_DONE,
    PIGEON_WGI_TIMER_DEPTH_PREPASS_DONE,
    PIGEON_WGI_TIMER_SHADOW_MAPS_DONE,
    PIGEON_WGI_TIMER_LIGHT_PASS_DONE,
    PIGEON_WGI_TIMER_LIGHT_GAUSSIAN_BLUR_DONE,
    PIGEON_WGI_TIMER_RENDER_DONE,
    PIGEON_WGI_TIMER_BLOOM_DOWNSAMPLE_DONE,
    PIGEON_WGI_TIMER_BLOOM_GAUSSIAN_BLUR_DONE,
    PIGEON_WGI_TIMER_POST_PROCESS_DONE,
    PIGEON_WGI_TIMERS_COUNT
} PigeonWGITimer;

PIGEON_ERR_RET pigeon_wgi_next_frame_wait(double delayed_timer_values[PIGEON_WGI_TIMERS_COUNT]);
PIGEON_ERR_RET pigeon_wgi_next_frame_poll(double delayed_timer_values[PIGEON_WGI_TIMERS_COUNT], bool* ready);

// Alignment (number of bones) of bone data for 1 armature in the bone uniform data
// This is to align uniform blocks when using OpenGL
// first_bone_offset must be a multiple of this
unsigned int pigeon_wgi_get_bone_data_alignment(void);

// max_draws determines the minimum size of the draws ssbo
// max_multidraw_draws = maximum number of draws within multidraw draws
// Instancing counts as multiple draws
// delayed_timer_values is set to the timer query results from 2 or more frames ago
// index into shadows = index into lights array in per-frame uniform data
PIGEON_ERR_RET pigeon_wgi_start_frame(uint32_t max_draws,
    uint32_t max_multidraw_draws,
    PigeonWGIShadowParameters shadows[4], unsigned int total_bones,
    PigeonWGIBoneMatrix ** bone_matrices);

PIGEON_ERR_RET pigeon_wgi_set_uniform_data(PigeonWGISceneUniformData * uniform_data, 
    PigeonWGIDrawObject *, unsigned int draws_count);

struct PigeonWGICommandBuffer;

typedef struct PigeonWGICommandBuffer PigeonWGICommandBuffer;

// TODO use enum instead?
// ! Discard the command buffer pointers after presenting the frame !
PigeonWGICommandBuffer * pigeon_wgi_get_upload_command_buffer(void);
PigeonWGICommandBuffer * pigeon_wgi_get_depth_command_buffer(void);
PigeonWGICommandBuffer * pigeon_wgi_get_shadow_command_buffer(unsigned int light_index);
PigeonWGICommandBuffer * pigeon_wgi_get_light_pass_command_buffer(void);
PigeonWGICommandBuffer * pigeon_wgi_get_render_command_buffer(void);

bool pigeon_wgi_multithreading_supported(void);

PIGEON_ERR_RET pigeon_wgi_start_command_buffer(PigeonWGICommandBuffer *);
void pigeon_wgi_draw_without_mesh(PigeonWGICommandBuffer*, PigeonWGIPipeline*, unsigned int vertices);
// pigeon_wgi_upload_multimesh

// first and count are either offsets into vertices or indices array depending on whether
//  the mesh has indices or not
// Requires 'instances' number of draw objects, starting at 'draw_index'
// diffuse_texture, nmap_texture, first_bone_index, bones_count are ignored if 
//  multidraw is supported (vulkan renderer) and can be set to -1.
//  If multidraw is not supported (opengl) then the textures are the same values
//  passed to wgi_bind_array_texture
void pigeon_wgi_draw(PigeonWGICommandBuffer*, PigeonWGIPipeline*, PigeonWGIMultiMesh*, 
    uint32_t start_vertex, uint32_t draw_index, uint32_t instances,
    uint32_t first, unsigned int count,
    int diffuse_texture, int nmap_texture, unsigned int first_bone_index, unsigned int bones_count);

bool pigeon_wgi_multidraw_supported(void);

void pigeon_wgi_multidraw_draw(unsigned int multidraw_draw_index, unsigned int start_vertex, uint32_t instances,
    uint32_t first, uint32_t count, uint32_t first_instance);

void pigeon_wgi_multidraw_submit(PigeonWGICommandBuffer*, PigeonWGIPipeline*, PigeonWGIMultiMesh*,
    uint32_t first_multidraw_index, uint32_t multidraw_count, uint32_t first_draw_index, uint32_t draws);

PIGEON_ERR_RET pigeon_wgi_end_command_buffer(PigeonWGICommandBuffer *);

// These 4 do nothing when using OpenGL

PIGEON_ERR_RET pigeon_wgi_present_frame_rec_sub0(void); // upload&depth&shadow

PIGEON_ERR_RET pigeon_wgi_present_frame_rec1(void); // light
PIGEON_ERR_RET pigeon_wgi_present_frame_rec2(void); // render
PIGEON_ERR_RET pigeon_wgi_present_frame_rec3(void); // post

// Blocks when using OpenGL
PIGEON_ERR_RET pigeon_wgi_present_frame_sub1(void); // everything else


// Returns 2 (fail) if the window is minimised or smaller than 16x16 pixels
PIGEON_ERR_RET pigeon_wgi_recreate_swapchain(void);
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
