#pragma once

#include "window.h"
#include <stdbool.h>
#include <stdint.h>
#include "pipeline.h"
#include "uniform.h"
#include "mesh.h"
#include "shadow.h"
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/types.h>
#include "rendergraph.h"

struct PigeonVulkanCommandPool;

/* Returns 0 on success. If fails, call pigeon_wgi_deinit() to cleanup */
ERROR_RETURN_TYPE pigeon_wgi_init(PigeonWindowParameters window_parameters, bool prefer_dedicated_gpu,
	PigeonWGIRenderConfig render_graph, float znear, float zfar);

void pigeon_wgi_set_depth_range(float znear, float zfar);

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

// Returns 1 on error, 2 if not ready yet, 3 if swapchain must be recreated
// If returns 3, call pigeon_wgi_recreate_swapchain()
// max_draw_calls determines the minimum size of the draw calls ssbo
// max_multidraw_draw_calls = maximum number of drawcalls within multidraw draws
// Instancing counts as multiple draw calls
// delayed_timer_values is set to the timer query results from 2 or more frames ago
// index into shadows = index into lights array in per-frame uniform data
ERROR_RETURN_TYPE pigeon_wgi_start_frame(bool block, uint32_t max_draw_calls,
    uint32_t max_multidraw_draw_calls, double delayed_timer_values[PIGEON_WGI_TIMERS_COUNT],
    PigeonWGIShadowParameters shadows[4]);

ERROR_RETURN_TYPE pigeon_wgi_set_uniform_data(PigeonWGISceneUniformData * uniform_data, 
    PigeonWGIDrawCallObject *, unsigned int num_draw_calls);

struct PigeonWGICommandBuffer;

typedef struct PigeonWGICommandBuffer PigeonWGICommandBuffer;

// ! Discard the command buffer pointers after presenting the frame !
PigeonWGICommandBuffer * pigeon_wgi_get_upload_command_buffer(void);
PigeonWGICommandBuffer * pigeon_wgi_get_depth_command_buffer(void);
PigeonWGICommandBuffer * pigeon_wgi_get_shadow_command_buffer(unsigned int light_index);
PigeonWGICommandBuffer * pigeon_wgi_get_light_pass_command_buffer(void);
PigeonWGICommandBuffer * pigeon_wgi_get_render_command_buffer(void);

ERROR_RETURN_TYPE pigeon_wgi_start_command_buffer(PigeonWGICommandBuffer *);
void pigeon_wgi_draw_without_mesh(PigeonWGICommandBuffer*, PigeonWGIPipeline*, unsigned int vertices);
// pigeon_wgi_upload_multimesh

// first and count are either offsets into vertices or indices array depending on whether
//  the mesh has indices or not
// Requires 'instances' number of draw call objects, starting at 'draw_call_index'
void pigeon_wgi_draw(PigeonWGICommandBuffer*, PigeonWGIPipeline*, PigeonWGIMultiMesh*, 
    uint32_t start_vertex, uint32_t draw_call_index, uint32_t instances,
    uint32_t first, unsigned int count);

void pigeon_wgi_multidraw_draw(unsigned int start_vertex, uint32_t instances, uint32_t first, uint32_t count);

void pigeon_wgi_multidraw_submit(PigeonWGICommandBuffer*, PigeonWGIPipeline*, PigeonWGIMultiMesh*,
    uint32_t first_multidraw_index, uint32_t drawcalls, uint32_t first_drawcall_index);

ERROR_RETURN_TYPE pigeon_wgi_end_command_buffer(PigeonWGICommandBuffer *);

// If returns 2, call pigeon_wgi_recreate_swapchain. Do *NOT* call present again; start the next frame.
ERROR_RETURN_TYPE pigeon_wgi_present_frame(bool debug_disable_bloom);


// Returns 2 (fail) if the window is minimised or smaller than 16x16 pixels
ERROR_RETURN_TYPE pigeon_wgi_recreate_swapchain(void);
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
