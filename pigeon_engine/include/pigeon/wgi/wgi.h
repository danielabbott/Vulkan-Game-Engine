#pragma once

#include "window.h"
#include <stdbool.h>
#include <stdint.h>
#include "pipeline.h"
#include "uniform.h"
#include "mesh.h"
#include <cglm/types.h>

struct PigeonVulkanCommandPool;

/* Returns 0 on success. If fails, call pigeon_wgi_deinit() to cleanup */
ERROR_RETURN_TYPE pigeon_wgi_init(PigeonWindowParameters window_parameters, bool prefer_dedicated_gpu);

bool pigeon_wgi_close_requested(void);

// Returns 1 on error, 2 if not ready yet, 3 if swapchain must be recreated
// If returns 3, call pigeon_wgi_recreate_swapchain()
// max_game_objects and max_materials determine the maximum size of the uniform data
ERROR_RETURN_TYPE pigeon_wgi_start_frame(bool block, unsigned int max_draw_calls);

ERROR_RETURN_TYPE pigeon_wgi_set_uniform_data(PigeonWGISceneUniformData * uniform_data, 
    PigeonWGIDrawCallObject *, unsigned int num_draw_calls);

struct PigeonWGICommandBuffer;

typedef struct PigeonWGICommandBuffer PigeonWGICommandBuffer;

// ! Discard the command buffer pointers after presenting the frame !
PigeonWGICommandBuffer * pigeon_wgi_get_upload_command_buffer(void);
PigeonWGICommandBuffer * pigeon_wgi_get_depth_command_buffer(void);
PigeonWGICommandBuffer * pigeon_wgi_get_render_command_buffer(void);

ERROR_RETURN_TYPE pigeon_wgi_start_command_buffer(PigeonWGICommandBuffer *);
void pigeon_wgi_draw_without_mesh(PigeonWGICommandBuffer*, PigeonWGIPipeline*, unsigned int vertices);
// pigeon_wgi_upload_mesh

// first and count are either offsets into vertices or indices array depending on whether
//  the mesh has indices or not
// Requires 'instances' number of draw call objects, starting at 'draw_call_index'
void pigeon_wgi_draw(PigeonWGICommandBuffer*, PigeonWGIPipeline*, PigeonWGIMultiMesh*, 
    uint64_t mesh_offset, PigeonWGIMeshMeta * meta, uint32_t draw_call_index, uint32_t instances,
    unsigned int first, unsigned int count);


ERROR_RETURN_TYPE pigeon_wgi_end_command_buffer(PigeonWGICommandBuffer *);

// If returns 2, call pigeon_wgi_recreate_swapchain. Do *NOT* call present again; start the next frame.
ERROR_RETURN_TYPE pigeon_wgi_present_frame(void);


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

void pigeon_wgi_perspective(mat4 m, float fovy, float aspect, float nearZ);
