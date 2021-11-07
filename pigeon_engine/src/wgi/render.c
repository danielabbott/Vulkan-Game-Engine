#include <pigeon/wgi/wgi.h>
#include <pigeon/assert.h>
#include <pigeon/wgi/vulkan/command.h>
#include <pigeon/wgi/vulkan/descriptor.h>
#include <pigeon/wgi/vulkan/image.h>
#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/wgi/vulkan/framebuffer.h>
#include <pigeon/wgi/vulkan/buffer.h>
#include <pigeon/wgi/vulkan/vulkan.h>
#include <pigeon/wgi/window.h>
#include <pigeon/wgi/uniform.h>
#include <pigeon/wgi/animation.h>
#include "singleton.h"
#include <pigeon/misc.h>
#include <string.h>
#include <pigeon/wgi/opengl/limits.h>
#include <pigeon/wgi/opengl/draw.h>
#include <pigeon/wgi/opengl/shader.h>
#include <pigeon/wgi/opengl/buffer.h>
#include <pigeon/wgi/textures.h>
#include <pigeon/wgi/wgi.h>
#include "tex.h"

static void create_render_stage_info(void)
{
    memset(singleton_data.stages, 0, sizeof singleton_data.stages);
    PigeonWGIRenderStageInfo * stage = &singleton_data.stages[PIGEON_WGI_RENDER_STAGE_UPLOAD];
    stage->active = true;

    for(unsigned int i = 0; i < 4; i++) {
        if(!singleton_data.shadow_parameters[i].resolution) continue;

        stage = &singleton_data.stages[PIGEON_WGI_RENDER_STAGE_SHADOW0+i]; 
        stage->render_mode = PIGEON_WGI_RENDER_STAGE_MODE_DEPTH_ONLY;
        stage->active = true;
        stage->mvp_index = 1+i;
        if(VULKAN) {
            stage->framebuffer = &singleton_data.shadow_framebuffers[singleton_data.shadow_parameters[i].framebuffer_index];
            stage->render_pass = &singleton_data.rp_depth;
        }
        else {
            stage->gl.framebuffer = &singleton_data.gl.shadow_framebuffers[singleton_data.shadow_parameters[i].framebuffer_index];
        }
    }

    stage = &singleton_data.stages[PIGEON_WGI_RENDER_STAGE_DEPTH];
    stage->render_mode = PIGEON_WGI_RENDER_STAGE_MODE_DEPTH_ONLY;
    stage->active = true;
    if(VULKAN) {
        stage->framebuffer = &singleton_data.depth_framebuffer;
        stage->render_pass = &singleton_data.rp_depth;
    }
    else {
        stage->gl.framebuffer = &singleton_data.gl.depth_framebuffer;
    }


    stage = &singleton_data.stages[PIGEON_WGI_RENDER_STAGE_SSAO];
    if(singleton_data.active_render_cfg.ssao) {
        stage->render_mode = PIGEON_WGI_RENDER_STAGE_MODE_FULL_SCREEN_PASS;
        stage->active = true;
    }


    stage = &singleton_data.stages[PIGEON_WGI_RENDER_STAGE_RENDER];
    stage->render_mode = PIGEON_WGI_RENDER_STAGE_MODE_NORMAL;
    stage->active = true;
    if(VULKAN) {
        stage->framebuffer = &singleton_data.render_framebuffer;
        stage->render_pass = &singleton_data.rp_render;
    }
    else {
        stage->gl.framebuffer = &singleton_data.gl.render_framebuffer;
    }


    stage = &singleton_data.stages[PIGEON_WGI_RENDER_STAGE_BLOOM];
    if(singleton_data.active_render_cfg.bloom) {
        stage->render_mode = PIGEON_WGI_RENDER_STAGE_MODE_FULL_SCREEN_PASS;
        stage->active = true;
    }


    stage = &singleton_data.stages[PIGEON_WGI_RENDER_STAGE_POST_AND_UI];
    stage->render_mode = PIGEON_WGI_RENDER_STAGE_MODE_FULL_SCREEN_PASS;
    stage->active = true;
}

PIGEON_ERR_RET pigeon_wgi_create_per_frame_objects()
{
    singleton_data.swapchain_image_index = UINT32_MAX;
    singleton_data.previous_frame_index_mod = UINT32_MAX;

    PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();
    singleton_data.frame_objects_count = sc_info.image_count;

    singleton_data.per_frame_objects = calloc(sc_info.image_count, sizeof *singleton_data.per_frame_objects);
    ASSERT_R1(singleton_data.per_frame_objects);

    if(VULKAN) {
        singleton_data.post_framebuffers = calloc(sc_info.image_count, sizeof *singleton_data.post_framebuffers);
        ASSERT_R1(singleton_data.post_framebuffers);
    }

    for(unsigned int i = 0; i < singleton_data.frame_objects_count; i++) {
        PerFrameData * objects = &singleton_data.per_frame_objects[i];

        if(VULKAN) {
            for(unsigned int j = 0; j < PIGEON_WGI_RENDER_STAGE__COUNT; j++) {
                bool one_shot = j != PIGEON_WGI_RENDER_STAGE_SSAO && j != PIGEON_WGI_RENDER_STAGE_BLOOM;
                ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->command_pools[j], 1, true, false, one_shot));
            }
        }

        
        // Uniform buffers are (re)created as needed and the descriptors are set there too


        if(VULKAN) {
            ASSERT_R1(!pigeon_vulkan_create_descriptor_pool(&objects->depth_descriptor_pool, 1, &singleton_data.depth_descriptor_layout));
            ASSERT_R1(!pigeon_vulkan_create_descriptor_pool(&objects->render_descriptor_pool, 1, &singleton_data.render_descriptor_layout));


            for(unsigned int j = 0; j < 4; j++) {
                pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 4, j, 
                    &singleton_data.default_shadow_map_image_view, &singleton_data.shadow_sampler);
            }


	        if(singleton_data.active_render_cfg.ssao) {
                pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 3, 0, 
                    &singleton_data.ssao_images[1].image_view, &singleton_data.bilinear_sampler);
            }
            else {
                pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 3, 0, 
                    &singleton_data.default_1px_white_texture_image_view, &singleton_data.nearest_filter_sampler);
            }


            for(unsigned int j = 0; j < 59; j++) {
                pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 5, j, 
                    &singleton_data.default_1px_white_texture_array_image_view, &singleton_data.texture_sampler);
            }

            ASSERT_R1(!pigeon_vulkan_create_fence(&objects->render_done_fence, false));

            for(unsigned int j = 0; j < sizeof objects->semaphores_all / sizeof objects->semaphores_all[0]; j++)
                ASSERT_R1(!pigeon_vulkan_create_semaphore(&objects->semaphores_all[j]));
            

            if(pigeon_vulkan_general_queue_supports_timestamps())
                ASSERT_R1(!pigeon_vulkan_create_timer_query_pool(&objects->timer_query_pool, 2*PIGEON_WGI_RENDER_STAGE__COUNT));
        

            // Framebuffer is specific to swapchain image - NOT current frame index mod

            ASSERT_R1(!pigeon_vulkan_create_framebuffer(&singleton_data.post_framebuffers[i],
                NULL, pigeon_vulkan_get_swapchain_image_view(i), &singleton_data.rp_post));
        }
        else {
            ASSERT_R1(!pigeon_opengl_create_timer_query_group(&objects->gl.timer_queries, 1+PIGEON_WGI_RENDER_STAGE__COUNT));
        }

    }
    return 0;
}

void pigeon_wgi_destroy_per_frame_objects()
{
    if(!singleton_data.per_frame_objects) return;

    if(VULKAN) {
        for(unsigned int i = 0; i < singleton_data.frame_objects_count; i++) {
            if(singleton_data.post_framebuffers[i].vk_framebuffer) pigeon_vulkan_destroy_framebuffer(&singleton_data.post_framebuffers[i]);

            PerFrameData * objects = &singleton_data.per_frame_objects[i];

            if(objects->timer_query_pool.vk_query_pool)
                pigeon_vulkan_destroy_timer_query_pool(&objects->timer_query_pool);

            for(unsigned int j = 0; j < sizeof objects->semaphores_all / sizeof objects->semaphores_all[0]; j++)
                pigeon_vulkan_destroy_semaphore(&objects->semaphores_all[j]);
                

            if(objects->render_done_fence.vk_fence) pigeon_vulkan_destroy_fence(&objects->render_done_fence);

            if(objects->depth_descriptor_pool.vk_descriptor_pool)
                pigeon_vulkan_destroy_descriptor_pool(&objects->depth_descriptor_pool);
            if(objects->render_descriptor_pool.vk_descriptor_pool)
                pigeon_vulkan_destroy_descriptor_pool(&objects->render_descriptor_pool);

            for(unsigned int j = 0; j < PIGEON_WGI_RENDER_STAGE__COUNT; j++)
                pigeon_vulkan_destroy_command_pool(&objects->command_pools[j]);

            if(objects->uniform_buffer.vk_buffer) pigeon_vulkan_destroy_buffer(&objects->uniform_buffer);
            if(objects->uniform_buffer_memory.vk_device_memory)
                pigeon_vulkan_free_memory(&objects->uniform_buffer_memory);
        }
    }

    if(OPENGL) {
        for(unsigned int i = 0; i < singleton_data.frame_objects_count; i++) {
            PerFrameData * objects = &singleton_data.per_frame_objects[i];

            pigeon_opengl_destroy_buffer(&objects->gl.uniform_buffer);
            pigeon_opengl_destroy_timer_query_group(&objects->gl.timer_queries);
        }
    }

    free_if(singleton_data.per_frame_objects);
    free_if(singleton_data.post_framebuffers);
}

static PIGEON_ERR_RET create_uniform_buffer(PigeonVulkanMemoryAllocation * memory, PigeonVulkanBuffer * buffer, unsigned int size)
{
	PigeonVulkanMemoryRequirements memory_req;

    PigeonVulkanBufferUsages usages = {0};
    usages.uniforms = true;
    usages.ssbo = true;
    usages.draw_indirect = true;

	ASSERT_R1 (!pigeon_vulkan_create_buffer(
		buffer,
		size,
        usages,
		&memory_req
	));

	PigeonVulkanMemoryTypePreferences preferences = { 0 };
	preferences.device_local = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED;
	preferences.host_visible = PIGEON_VULKAN_MEMORY_TYPE_MUST;
	preferences.host_coherent = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED;
	preferences.host_cached = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT;

	ASSERT_R1 (!pigeon_vulkan_allocate_memory(memory, memory_req, preferences));
    ASSERT_R1 (!pigeon_vulkan_map_memory(memory, NULL));

	ASSERT_R1 (!pigeon_vulkan_buffer_bind_memory(buffer, memory, 0));
	return 0;
}


static PIGEON_ERR_RET prepare_uniform_buffers()
{
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    const unsigned int align = VULKAN ? pigeon_vulkan_get_buffer_min_alignment()
        : pigeon_opengl_get_uniform_buffer_min_alignment();

    unsigned int minimum_size = round_up(sizeof(PigeonWGISceneUniformData), align);

    if(VULKAN) {
        minimum_size += round_up(sizeof(PigeonWGIDrawObject) * singleton_data.max_draws, align);
        minimum_size += round_up(sizeof(PigeonVulkanDrawIndexedIndirectCommand) * singleton_data.max_multidraw_draws, align);     
    }
    else {
        minimum_size += round_up(sizeof(PigeonWGIDrawObject), align) * singleton_data.max_draws;
    }
    minimum_size += (singleton_data.total_bones+256) * sizeof(PigeonWGIBoneMatrix);

    if(VULKAN) {
        if(objects->uniform_buffer.size < minimum_size) {
            if(objects->uniform_buffer.size) {
                pigeon_vulkan_destroy_buffer(&objects->uniform_buffer);
                pigeon_vulkan_free_memory(&objects->uniform_buffer_memory);
            }

            ASSERT_R1(!create_uniform_buffer(&objects->uniform_buffer_memory, 
                &objects->uniform_buffer, minimum_size));

            unsigned int uniform_offset = 0;
            
                
            pigeon_vulkan_set_descriptor_uniform_buffer2(&objects->render_descriptor_pool, 0, 0, 0,
                &objects->uniform_buffer, uniform_offset, sizeof(PigeonWGISceneUniformData));
            pigeon_vulkan_set_descriptor_uniform_buffer2(&objects->depth_descriptor_pool, 0, 0, 0,
                &objects->uniform_buffer, uniform_offset, sizeof(PigeonWGISceneUniformData));

            uniform_offset += round_up(sizeof(PigeonWGISceneUniformData), align);
                
            pigeon_vulkan_set_descriptor_ssbo2(&objects->render_descriptor_pool, 0, 1, 0,
                &objects->uniform_buffer, uniform_offset,
                sizeof(PigeonWGIDrawObject) * singleton_data.max_draws);
            pigeon_vulkan_set_descriptor_ssbo2(&objects->depth_descriptor_pool, 0, 1, 0,
                &objects->uniform_buffer, uniform_offset,
                sizeof(PigeonWGIDrawObject) * singleton_data.max_draws);
                
            uniform_offset += round_up(sizeof(PigeonWGIDrawObject) * singleton_data.max_draws, align);
            uniform_offset += round_up(sizeof(PigeonVulkanDrawIndexedIndirectCommand) * singleton_data.max_multidraw_draws, align);
                
            pigeon_vulkan_set_descriptor_ssbo2(&objects->render_descriptor_pool, 0, 2, 0,
                &objects->uniform_buffer, uniform_offset, singleton_data.total_bones * sizeof(PigeonWGIBoneMatrix));
            pigeon_vulkan_set_descriptor_ssbo2(&objects->depth_descriptor_pool, 0, 2, 0,
                &objects->uniform_buffer, uniform_offset, singleton_data.total_bones * sizeof(PigeonWGIBoneMatrix));
        }
    }
    else {
        if(objects->gl.uniform_buffer.size < minimum_size) {
            if(objects->gl.uniform_buffer.size) {
                pigeon_opengl_destroy_buffer(&objects->gl.uniform_buffer);
            }

            ASSERT_R1(!pigeon_opengl_create_buffer(&objects->gl.uniform_buffer, minimum_size, 
                PIGEON_OPENGL_BUFFER_TYPE_UNIFORM));
        }
    }
    


    return 0;
}


PIGEON_ERR_RET pigeon_wgi_next_frame_wait(double delayed_timer_values[PIGEON_WGI_RENDER_STAGE__COUNT])
{
    return pigeon_wgi_next_frame_poll(delayed_timer_values, NULL);
}

static PIGEON_ERR_RET recreate_swapchain_loop(void)
{
    while (true) {
        int err = pigeon_wgi_recreate_swapchain();
        if(!err) break;
        else if (err == 2) {
            pigeon_wgi_wait_events();
        }
        else {
            return 1;
        }
    }
    return 0;
}

static PIGEON_ERR_RET maybe_recreate_swapchain_loop(bool block, bool* ready)
{
    if(block) {
        ASSERT_R1(!recreate_swapchain_loop());         
    }
    else {
        int err = pigeon_wgi_recreate_swapchain();
        if(err == 2) {
            *ready = false;
            return 0;
        }
        ASSERT_R1(!err);
    }
    return 0;
}

PIGEON_ERR_RET pigeon_wgi_next_frame_poll(double delayed_timer_values[PIGEON_WGI_RENDER_STAGE__COUNT], bool* ready)
{
    pigeon_wgi_poll_events();
    bool block = ready == NULL;
        
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    if(VULKAN) {
        if(objects->commands_in_progress) {
            if(block) {
                ASSERT_R1(!pigeon_vulkan_wait_fence(&objects->render_done_fence));
                ASSERT_R1(!pigeon_vulkan_reset_fence(&objects->render_done_fence));
                objects->commands_in_progress = false;
            }
            else {
                bool fence_state;
                ASSERT_R1(!pigeon_vulkan_poll_fence(&objects->render_done_fence, &fence_state));

                if(fence_state) {
                    ASSERT_R1(!pigeon_vulkan_reset_fence(&objects->render_done_fence));
                    objects->commands_in_progress = false;
                }
                else {
                    *ready = false;
                    return 0;
                }
            }
        }
    }


    unsigned int window_width, window_height;
    pigeon_wgi_get_window_dimensions(&window_width, &window_height);    

    while (window_width < 16 || window_height < 16) {
        if(block) pigeon_wgi_wait_events();
        else {
            *ready = false;
            return 0;
        }
        pigeon_wgi_get_window_dimensions(&window_width, &window_height);  
    }

    PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();

    if(OPENGL) {
        if(sc_info.width != singleton_data.render_image.gltex2d.width ||
            sc_info.height != singleton_data.render_image.gltex2d.height)
        {
            ASSERT_R1(!maybe_recreate_swapchain_loop(block, ready));
        }
        
        if(ready) *ready = true;

        objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
        if(!objects->first_frame_submitted) {        
            memset(delayed_timer_values, 0, sizeof(double) * PIGEON_WGI_RENDER_STAGE__COUNT);
        }
        else {
            double prev_time = pigeon_opengl_get_timer_query_result(&objects->gl.timer_queries, 0);

            for(unsigned int i = 0; i < PIGEON_WGI_RENDER_STAGE__COUNT; i++) {
                if(singleton_data.stages[i].active) {
                    double t = pigeon_opengl_get_timer_query_result(&objects->gl.timer_queries, 1+i);
                    delayed_timer_values[i] = t - prev_time;
                    prev_time = t;
                }
                else {
                    delayed_timer_values[i] = 0.0;
                }
            }
        }


        return 0;
    }





    int swapchain_err = pigeon_vulkan_next_swapchain_image(&singleton_data.swapchain_image_index, 
        &objects->semaphores.swapchain_aquisition, NULL, block);

    if(ready) *ready = true;

    if(swapchain_err == 3) {
        // not ready yet
        ASSERT_R1(!block);
        *ready = false;
        return 0;
    }
    else if(swapchain_err == 2) {
        ASSERT_R1(!maybe_recreate_swapchain_loop(block, ready));
    }
    else {
        ASSERT_R1(!swapchain_err);
    }

    if(!ready || *ready == true) {
        double absolute_times[2*PIGEON_WGI_RENDER_STAGE__COUNT];

        if(!pigeon_vulkan_general_queue_supports_timestamps() || !objects->first_frame_submitted ||
            pigeon_vulkan_get_timer_results(&objects->timer_query_pool, absolute_times))
        {        
            memset(delayed_timer_values, 0, sizeof(double) * PIGEON_WGI_RENDER_STAGE__COUNT);
        }

        for(unsigned int i = 0; i < PIGEON_WGI_RENDER_STAGE__COUNT; i++) {
            delayed_timer_values[i] = absolute_times[2*i+1] - absolute_times[2*i];
        }
    }

    return 0;
    
}

unsigned int pigeon_wgi_get_draw_data_alignment(void)
{
    if(VULKAN) return 1;
    else return pigeon_opengl_get_uniform_buffer_min_alignment();
}

unsigned int pigeon_wgi_get_bone_data_alignment(void)
{
    if(VULKAN) return 1;
    else return (pigeon_opengl_get_uniform_buffer_min_alignment() + 4*3*4 - 1) / (4*3*4);
}

PIGEON_ERR_RET pigeon_wgi_start_frame(unsigned int max_draws,
    uint32_t max_multidraw_draws,
    PigeonWGIShadowParameters shadows[4], unsigned int total_bones,
    void ** draw_objects,
    PigeonWGIBoneMatrix ** bone_matrices)
{
    ASSERT_R1(draw_objects && bone_matrices);
    ASSERT_R1(max_draws <= 65536);
    ASSERT_R1(total_bones <= max_draws*256);
    ASSERT_R1(max_multidraw_draws <= max_draws);

    if(max_draws > singleton_data.max_draws) singleton_data.max_draws = max_draws;
    if(!singleton_data.max_draws) singleton_data.max_draws = 128;
    
    if(max_multidraw_draws > singleton_data.max_multidraw_draws)
        singleton_data.max_multidraw_draws = max_multidraw_draws;
    if(!singleton_data.max_multidraw_draws) singleton_data.max_multidraw_draws = 128;

    if(OPENGL) singleton_data.max_multidraw_draws = 0;
    
    if(total_bones > singleton_data.total_bones) singleton_data.total_bones = total_bones;

    
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod]; 


    memcpy(singleton_data.shadow_parameters, shadows, sizeof singleton_data.shadow_parameters);
    ASSERT_R1(!pigeon_wgi_assign_shadow_framebuffers());
    
    objects->commands_in_progress = true;
    create_render_stage_info();
    ASSERT_R1(!prepare_uniform_buffers());


    // Bind shadow textures

    if(VULKAN) {
        for(unsigned int i = 0; i < 4; i++) {
            PigeonWGIShadowParameters* p = &singleton_data.shadow_parameters[i];
            if(p->resolution) {
                unsigned int j = (unsigned)p->framebuffer_index;
                pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 4, i, 
                    &singleton_data.shadow_images[j].image_view, &singleton_data.shadow_sampler);
            }
            else {
                pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 4, i, 
                    &singleton_data.default_shadow_map_image_view, &singleton_data.shadow_sampler);
            }
        }

        const unsigned int align = pigeon_vulkan_get_buffer_min_alignment();

        uint8_t * dst = objects->uniform_buffer_memory.mapping;
        dst += round_up(sizeof(PigeonWGISceneUniformData), align);

        *draw_objects = dst;

        dst += round_up(sizeof(PigeonWGIDrawObject) * singleton_data.max_draws, align);
        dst += round_up(sizeof(PigeonVulkanDrawIndexedIndirectCommand) * singleton_data.max_multidraw_draws, align);


        *bone_matrices = (PigeonWGIBoneMatrix* ) (void*)dst;
    }
    else {
        pigeon_opengl_bind_uniform_buffer2(&objects->gl.uniform_buffer, 0, 0, 
            sizeof(PigeonWGISceneUniformData));

        for(unsigned int i = 0; i < 4; i++) {
            PigeonWGIShadowParameters* p = &singleton_data.shadow_parameters[i];
            if(p->resolution) {
                pigeon_opengl_bind_texture(i, &singleton_data.shadow_images[i].gltex2d);
            }
            else {
                pigeon_opengl_bind_texture(i, &singleton_data.gl.default_shadow_map_image);
            }
        }
        const unsigned int align = pigeon_opengl_get_uniform_buffer_min_alignment();
    
        pigeon_opengl_bind_uniform_buffer(&objects->gl.uniform_buffer);
        ASSERT_R1(pigeon_opengl_map_buffer(&objects->gl.uniform_buffer));


        uint8_t * dst = objects->gl.uniform_buffer.mapping;
        dst += round_up(sizeof(PigeonWGISceneUniformData), align);

        *draw_objects = dst;

        dst += round_up(sizeof(PigeonWGIDrawObject), align) * singleton_data.max_draws;

        *bone_matrices = (PigeonWGIBoneMatrix *) (void *) dst;

        pigeon_opengl_set_timer_query_value(&objects->gl.timer_queries, 0);
    }    

    return 0;
}

PIGEON_ERR_RET pigeon_wgi_set_uniform_data(PigeonWGISceneUniformData * uniform_data)
{
    uniform_data->znear = singleton_data.znear;
    uniform_data->zfar = singleton_data.zfar;
    memcpy(uniform_data->ambient, singleton_data.ambient, 3*4);
    
    pigeon_wgi_set_shadow_uniforms(uniform_data);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    uint8_t * dst = VULKAN ? objects->uniform_buffer_memory.mapping : 
        objects->gl.uniform_buffer.mapping;

    memcpy(dst, uniform_data, sizeof(PigeonWGISceneUniformData));

    if(VULKAN) {
        ASSERT_R1(!pigeon_vulkan_flush_memory(&objects->uniform_buffer_memory, 0, 0));
    }
    else {
        pigeon_opengl_bind_uniform_buffer(&objects->gl.uniform_buffer);
        pigeon_opengl_unmap_buffer(&objects->gl.uniform_buffer);

        pigeon_opengl_bind_uniform_buffer2(&objects->gl.uniform_buffer, 0, 0, 
            sizeof(PigeonWGISceneUniformData));
    }
    

    return 0;
}


bool pigeon_wgi_multithreading_supported(void)
{
    return VULKAN;
}


typedef void* PipelineOrProgram;

static PipelineOrProgram get_pipeline(PigeonWGIRenderStage stage, PigeonWGIPipeline* pipeline)
{
    const PigeonWGIRenderStageInfo * info = &singleton_data.stages[stage];
    if(VULKAN) {
        if(info->render_mode == PIGEON_WGI_RENDER_STAGE_MODE_DEPTH_ONLY) {
            if(stage >= PIGEON_WGI_RENDER_STAGE_SHADOW0 && stage <= PIGEON_WGI_RENDER_STAGE_SHADOW3) {
                return pipeline->pipeline_shadow_map;
            }
            return pipeline->pipeline_depth;
        }
        return pipeline->pipeline;
    }
    else {
        if(info->render_mode == PIGEON_WGI_RENDER_STAGE_MODE_DEPTH_ONLY) {
            return pipeline->gl.program_depth;
        }
        return pipeline->gl.program;
    }
}

void pigeon_wgi_draw_without_mesh(PigeonWGIRenderStage stage, PigeonWGIPipeline* pipeline, 
    unsigned int vertices)
{
    assert(pipeline);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    if(VULKAN) {
        PigeonVulkanCommandPool * p = &objects->command_pools[stage];
        assert(p->recording);
        pigeon_vulkan_bind_pipeline(p, 0, pipeline->pipeline);
        pigeon_vulkan_bind_descriptor_set(p, 0, pipeline->pipeline, 
            &objects->render_descriptor_pool, 0);

        pigeon_vulkan_draw(p, 0, 0, vertices, 1,
            get_pipeline(stage, pipeline), 0, NULL);
    }
    else {
        pigeon_opengl_bind_shader_program(get_pipeline(stage, pipeline));
        pigeon_opengl_set_draw_state(&pipeline->gl.config);

        pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);
    }
}

static void draw_setup_common_gl(PigeonWGIRenderStage stage, PigeonWGIPipeline* pipeline, 
    uint32_t draw_index, int diffuse_texture, int nmap_texture, 
    unsigned int first_bone_index, unsigned int bones_count)
{
    const PigeonWGIRenderStageInfo * info = &singleton_data.stages[stage];

    // Bind shader, set mvp index uniform

    PigeonOpenGLShaderProgram * program = NULL;
    int location = -1;
    int * value_cache = NULL;
    PigeonWGIPipelineConfig const* cfg;

    if(info->render_mode == PIGEON_WGI_RENDER_STAGE_MODE_DEPTH_ONLY) {
        program = pipeline->gl.program_depth;
        location = pipeline->gl.program_depth_mvp_loc;
        value_cache = &pipeline->gl.program_depth_mvp;
        cfg = &pipeline->gl.config_depth;

        if(stage >= PIGEON_WGI_RENDER_STAGE_SHADOW0 && stage <= PIGEON_WGI_RENDER_STAGE_SHADOW3) {
            cfg = &pipeline->gl.config_shadow;
        }
    }
    else {
        program = pipeline->gl.program;
        location = pipeline->gl.program_mvp_loc;
        value_cache = &pipeline->gl.program_mvp;
        cfg = &pipeline->gl.config;
    }

    pigeon_opengl_bind_shader_program(program);
    if(*value_cache != (int) info->mvp_index) {
        pigeon_opengl_set_uniform_i(program, location, (int) info->mvp_index);
        *value_cache = (int) info->mvp_index;
    }
    pigeon_opengl_set_draw_state(cfg);

    // Bind draw data

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    const unsigned int align = pigeon_opengl_get_uniform_buffer_min_alignment();

    unsigned int o = round_up(sizeof(PigeonWGISceneUniformData), align) +
        draw_index * round_up(sizeof(PigeonWGIDrawObject), align);

    pigeon_opengl_bind_uniform_buffer2(&objects->gl.uniform_buffer, 1, o, sizeof(PigeonWGIDrawObject));

    o = round_up(sizeof(PigeonWGISceneUniformData), align) +
        round_up(sizeof(PigeonWGIDrawObject), align) * singleton_data.max_draws +
        first_bone_index * sizeof(PigeonWGIBoneMatrix);
    
    if(bones_count)
        pigeon_opengl_bind_uniform_buffer2(&objects->gl.uniform_buffer, 2, o, 256*sizeof(PigeonWGIBoneMatrix));


    // Bind textures

    if(diffuse_texture >= 0 && singleton_data.gl.bound_textures[diffuse_texture])
        pigeon_opengl_bind_texture(4, &singleton_data.gl.bound_textures[diffuse_texture]->data->gl.texture);
    if(nmap_texture >= 0 && singleton_data.gl.bound_textures[nmap_texture])
        pigeon_opengl_bind_texture(6, &singleton_data.gl.bound_textures[nmap_texture]->data->gl.texture);
    
    
}

static void draw_setup_common(PigeonWGIRenderStage stage, PigeonWGIPipeline* pipeline, 
    PipelineOrProgram * vpipeline, PigeonWGIMultiMesh* mesh, uint32_t draw_index, 
    int diffuse_texture, int nmap_texture, unsigned int first_bone_index, unsigned int bones_count)
{
    if(OPENGL) { 
        *vpipeline = NULL;
        draw_setup_common_gl(stage, pipeline, draw_index, diffuse_texture, nmap_texture, first_bone_index, bones_count); 
        return; 
    }

    *vpipeline = get_pipeline(stage, pipeline);
    
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    const PigeonWGIRenderStageInfo * info = &singleton_data.stages[stage];
    PigeonVulkanCommandPool * p = &objects->command_pools[stage];
    assert(p->recording);

    pigeon_vulkan_bind_pipeline(p, 0, *vpipeline);

    
    pigeon_vulkan_bind_descriptor_set(p, 0, *vpipeline, 
        info->render_mode == PIGEON_WGI_RENDER_STAGE_MODE_DEPTH_ONLY ?
            (pipeline->transparent ? &objects->render_descriptor_pool : &objects->depth_descriptor_pool) : 
            &objects->render_descriptor_pool, 0);

    unsigned int attribute_count = 0;
    for (; attribute_count < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES; attribute_count++) {
        if (!mesh->attribute_types[attribute_count]) break;
    }

    pigeon_vulkan_bind_vertex_buffers(p, 0, &mesh->staged_buffer->buffer,
        attribute_count, mesh->attribute_start_offsets);
        
    if(mesh->index_count) {
        pigeon_vulkan_bind_index_buffer(p, 0, 
            &mesh->staged_buffer->buffer, mesh->index_data_offset, mesh->big_indices);
    }
}

void pigeon_wgi_draw(PigeonWGIRenderStage stage, PigeonWGIPipeline* pipeline, 
    PigeonWGIMultiMesh* mesh, uint32_t start_vertex,
    uint32_t draw_index, uint32_t instances, uint32_t first, uint32_t count,
    int diffuse_texture, int nmap_texture, unsigned int first_bone_index, unsigned int bones_count)
{
    assert(pipeline && mesh);
    if(VULKAN) assert(pipeline->pipeline && mesh->staged_buffer);
    if(OPENGL) assert(pipeline->gl.program && mesh->opengl_vao);

    if(!instances) instances = 1;
    if(!count) return;

    if(OPENGL && instances > 1) {
        assert(false);
        return;
    }

    PipelineOrProgram vpipeline;
    draw_setup_common(stage, pipeline, &vpipeline, mesh, draw_index, diffuse_texture, nmap_texture, first_bone_index, bones_count);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    const PigeonWGIRenderStageInfo * info = &singleton_data.stages[stage];
    uint32_t pushc[2] = {draw_index, info->mvp_index};
    
    if(mesh->index_count) {
        if(count == UINT32_MAX) {
            count = mesh->index_count - first;
        }

        if(VULKAN) pigeon_vulkan_draw_indexed(&objects->command_pools[stage], 0, 
            start_vertex, first, count, instances, vpipeline, sizeof pushc, &pushc);
        else pigeon_opengl_draw_indexed(mesh->opengl_vao, start_vertex, first, count);
    }
    else {
        if(count == UINT32_MAX) {
            count = mesh->vertex_count - first;
        }

        assert(!start_vertex);

        if(VULKAN) pigeon_vulkan_draw(&objects->command_pools[stage], 0, 
            first, count, instances, vpipeline, sizeof pushc, &pushc);
        else pigeon_opengl_draw(mesh->opengl_vao, first, count);
    }
}


bool pigeon_wgi_multidraw_supported(void)
{
    return VULKAN;
}

void pigeon_wgi_multidraw_draw(unsigned int multidraw_draw_index, unsigned int start_vertex,
	uint32_t instances, uint32_t first, uint32_t count, uint32_t first_instance)
{
    if(!instances || !count || !VULKAN) {
        assert(false);
        return;
    }

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    const unsigned int align = VULKAN ? pigeon_vulkan_get_buffer_min_alignment()
        : pigeon_opengl_get_uniform_buffer_min_alignment();

    unsigned int o = round_up(sizeof(PigeonWGISceneUniformData), align);

    if(VULKAN)
        o += round_up(sizeof(PigeonWGIDrawObject) * singleton_data.max_draws, align);
    else
        o += round_up(sizeof(PigeonWGIDrawObject), align) * singleton_data.max_draws;

    PigeonVulkanDrawIndexedIndirectCommand* indirect_cmds =
        (PigeonVulkanDrawIndexedIndirectCommand*)((uintptr_t)objects->uniform_buffer_memory.mapping + o);

    assert(multidraw_draw_index <= singleton_data.max_multidraw_draws);
    PigeonVulkanDrawIndexedIndirectCommand * cmd = &indirect_cmds[multidraw_draw_index];

    
    cmd->indexCount = count;
    cmd->instanceCount = instances;
    cmd->firstIndex = first;
    cmd->vertexOffset = (int32_t)start_vertex;
    cmd->firstInstance = first_instance;
}

void pigeon_wgi_multidraw_submit(PigeonWGIRenderStage stage,
    PigeonWGIPipeline* pipeline, PigeonWGIMultiMesh* mesh,
    uint32_t first_multidraw_index, uint32_t multidraw_count)
{
    if(!VULKAN) {
        assert(false);
        return;
    }

    assert(pipeline && pipeline->pipeline && mesh && mesh->staged_buffer);
    assert(first_multidraw_index + multidraw_count <= singleton_data.max_multidraw_draws);
    assert(mesh->index_count && mesh->vertex_count);

    PipelineOrProgram vpipeline;
    draw_setup_common(stage, pipeline, &vpipeline, mesh, 0, -1, -1, 0, 0);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    
    const unsigned int align = VULKAN ? pigeon_vulkan_get_buffer_min_alignment()
        : pigeon_opengl_get_uniform_buffer_min_alignment();
    unsigned int o = round_up(sizeof(PigeonWGISceneUniformData), align);
    o += round_up(sizeof(PigeonWGIDrawObject) * singleton_data.max_draws, align);

    const PigeonWGIRenderStageInfo * info = &singleton_data.stages[stage];
    uint32_t pushc[2] = {0, info->mvp_index};

    pigeon_vulkan_multidraw_indexed(&objects->command_pools[stage], 0, 
        vpipeline, sizeof pushc, &pushc,
        &objects->uniform_buffer, o,
        first_multidraw_index, multidraw_count);
}


bool pigeon_wgi_ssao_record_needed(void)
{
    if(OPENGL) return true;
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    return !objects->command_pools[PIGEON_WGI_RENDER_STAGE_SSAO].recorded;
}

bool pigeon_wgi_bloom_record_needed(void)
{
    if(OPENGL) return true;
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    return !objects->command_pools[PIGEON_WGI_RENDER_STAGE_BLOOM].recorded;
}

static PIGEON_ERR_RET pigeon_wgi_start_record_gl(PigeonWGIRenderStage stage)
{
    PigeonWGIRenderStageInfo* stage_info = &singleton_data.stages[stage];

    if(stage_info->render_mode == PIGEON_WGI_RENDER_STAGE_MODE_NO_RENDER) return 0;


    if(stage_info->render_mode == PIGEON_WGI_RENDER_STAGE_MODE_FULL_SCREEN_PASS) {
        ASSERT_R1(stage == PIGEON_WGI_RENDER_STAGE_POST_AND_UI);
    }


    if(stage_info->render_mode == PIGEON_WGI_RENDER_STAGE_MODE_DEPTH_ONLY) {
        pigeon_opengl_bind_framebuffer(stage_info->gl.framebuffer);
        pigeon_opengl_clear_depth();
    }    
    else if(stage_info->render_mode == PIGEON_WGI_RENDER_STAGE_MODE_NORMAL) {
        pigeon_opengl_bind_framebuffer(stage_info->gl.framebuffer);

        if(stage == PIGEON_WGI_RENDER_STAGE_RENDER) {
            for(unsigned int i = 0; i < 4; i++) { 
                if(singleton_data.shadow_images[i].gltex2d.id)
                    pigeon_opengl_bind_texture(i, &singleton_data.shadow_images[i].gltex2d);  
            }
            if(singleton_data.active_render_cfg.ssao)
                pigeon_opengl_bind_texture(5, &singleton_data.ssao_images[1].gltex2d);
        }
    }
    else if(stage == PIGEON_WGI_RENDER_STAGE_POST_AND_UI) {
        PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();
        
        pigeon_opengl_set_draw_state(&singleton_data.gl.full_screen_tri_cfg);
        pigeon_opengl_bind_framebuffer(NULL);
        pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_post);
        pigeon_opengl_bind_texture(0, &singleton_data.render_image.gltex2d);
        pigeon_opengl_bind_texture(1, &singleton_data.bloom_images[0][0].gltex2d);


        pigeon_opengl_set_uniform_vec3(&singleton_data.gl.shader_post, 
            singleton_data.gl.shader_post_u_one_pixel_and_bloom_intensity,
            1 / (float)sc_info.width, 1 / (float)sc_info.height,
            singleton_data.bloom_intensity               
        );

        pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);
    }
    

    return 0;
}

PIGEON_ERR_RET pigeon_wgi_start_record(PigeonWGIRenderStage stage)
{
    ASSERT_R1(stage != PIGEON_WGI_RENDER_STAGE_SSAO && stage != PIGEON_WGI_RENDER_STAGE_BLOOM);
    ASSERT_R1(singleton_data.stages[stage].active);
    if(OPENGL) return pigeon_wgi_start_record_gl(stage);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    PigeonVulkanCommandPool * p = &objects->command_pools[stage];
    ASSERT_R1(!p->recording);

    ASSERT_R1(!pigeon_vulkan_reset_command_pool(p));

    PigeonWGIRenderStageInfo* stage_info = &singleton_data.stages[stage];

    ASSERT_R1(!pigeon_vulkan_start_submission(p, 0));
    
    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        if(stage == PIGEON_WGI_RENDER_STAGE_UPLOAD) {
            pigeon_vulkan_reset_query_pool(p, 0, &objects->timer_query_pool);
            for(unsigned int i = 0; i < PIGEON_WGI_RENDER_STAGE__COUNT; i++) {
                if(!singleton_data.stages[i].active) {
                    pigeon_vulkan_set_timer(p, 0, &objects->timer_query_pool, 2*i);
                    pigeon_vulkan_set_timer(p, 0, &objects->timer_query_pool, 1+2*i);
                }
            }
        }
        pigeon_vulkan_set_timer(p, 0, &objects->timer_query_pool, 2*stage);

    }
    
    if(stage_info->render_mode == PIGEON_WGI_RENDER_STAGE_MODE_NO_RENDER) {
        return 0;
    }
    
    PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();
    
    unsigned int vp_w, vp_h;
    PigeonVulkanFramebuffer * fb = NULL;

    if(stage_info->render_mode == PIGEON_WGI_RENDER_STAGE_MODE_FULL_SCREEN_PASS) {
        ASSERT_R1(stage == PIGEON_WGI_RENDER_STAGE_POST_AND_UI);
    }
    
    if(stage_info->render_mode != PIGEON_WGI_RENDER_STAGE_MODE_FULL_SCREEN_PASS && stage_info->framebuffer) {
        vp_w = stage_info->framebuffer->width;
        vp_h = stage_info->framebuffer->height;
        fb = stage_info->framebuffer;
    }
    else {
        vp_w = sc_info.width;
        vp_h = sc_info.height;
        fb = &singleton_data.post_framebuffers[singleton_data.swapchain_image_index];
    }

    pigeon_vulkan_set_viewport_size(p, 0, vp_w, vp_h);

    if(stage_info->render_mode != PIGEON_WGI_RENDER_STAGE_MODE_FULL_SCREEN_PASS) {
        pigeon_vulkan_start_render_pass(p, 0, stage_info->render_pass, fb, vp_w, vp_h, false);
    }
    

    if(stage == PIGEON_WGI_RENDER_STAGE_POST_AND_UI) {
        // Post-process

        struct {
            float one_pixel_x, one_pixel_y;
            float bloom_intensity;
        } post_pushc;
        post_pushc.one_pixel_x = 1 / (float)sc_info.width;
        post_pushc.one_pixel_y = 1 / (float)sc_info.height;
        post_pushc.bloom_intensity = singleton_data.bloom_intensity;

        pigeon_vulkan_start_render_pass(p, 0, &singleton_data.rp_post, fb, vp_w, vp_h, false);
        pigeon_vulkan_bind_pipeline(p, 0, &singleton_data.pipeline_post);
        pigeon_vulkan_bind_descriptor_set(p, 0, &singleton_data.pipeline_post, &singleton_data.post_process_descriptor_pool, 0);
        pigeon_vulkan_draw(p, 0, 0, 3, 1, &singleton_data.pipeline_post, sizeof post_pushc, &post_pushc);
    }

    return 0;
}

PIGEON_ERR_RET pigeon_wgi_end_record(PigeonWGIRenderStage stage)
{
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    ASSERT_R1(singleton_data.stages[stage].active);

    if(OPENGL) {
        pigeon_opengl_set_timer_query_value(&objects->gl.timer_queries, 1+stage);
        return 0;
    }

    ASSERT_R1(stage != PIGEON_WGI_RENDER_STAGE_SSAO && stage != PIGEON_WGI_RENDER_STAGE_BLOOM);

    PigeonVulkanCommandPool * p = &objects->command_pools[stage];
    ASSERT_R1(!p->recorded);
    ASSERT_R1(p->recording);

    if(singleton_data.stages[stage].render_mode != PIGEON_WGI_RENDER_STAGE_MODE_NO_RENDER)
        pigeon_vulkan_end_render_pass(p, 0);
    
    if(pigeon_vulkan_general_queue_supports_timestamps())
        pigeon_vulkan_set_timer(p, 0, &objects->timer_query_pool, 1+2*stage);
    ASSERT_R1(!pigeon_vulkan_end_submission(p, 0));

    return 0;
}


PIGEON_ERR_RET pigeon_wgi_record_ssao(void)
{
    if(!singleton_data.active_render_cfg.ssao) return 0;

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();

    if(OPENGL) {
        // SSAO

        pigeon_opengl_set_draw_state(&singleton_data.gl.full_screen_tri_cfg);
        pigeon_opengl_bind_framebuffer(&singleton_data.gl.ssao_framebuffers[0]);
        pigeon_opengl_bind_texture(0, &singleton_data.depth_image.gltex2d);
        pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_ssao);
        pigeon_opengl_set_uniform_vec3(&singleton_data.gl.shader_ssao, singleton_data.gl.shader_ssao_u_near_far_cutoff,
            singleton_data.znear, singleton_data.zfar, singleton_data.ssao_cutoff
            );
        pigeon_opengl_set_uniform_vec2(&singleton_data.gl.shader_ssao, singleton_data.gl.shader_ssao_ONE_PIXEL,
            1 / (float) sc_info.width, 1 / (float) sc_info.height);
        pigeon_opengl_set_draw_state(&singleton_data.gl.full_screen_tri_cfg);
        pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);


        // SSAO downscale
        pigeon_opengl_bind_framebuffer(&singleton_data.gl.ssao_framebuffers[1]);
        pigeon_opengl_bind_texture(0, &singleton_data.ssao_images[0].gltex2d);
        pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_ssao_downscale_x4);
        pigeon_opengl_set_uniform_vec2(&singleton_data.gl.shader_ssao_downscale_x4,
            singleton_data.gl.shader_ssao_downscale_x4_OFFSET,
            1 / (float) sc_info.width, 1 / (float) sc_info.height);
        pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);

        // SSAO blur

        unsigned int w = sc_info.width / 4;
        unsigned int h = sc_info.height / 4;

        float blur_pushc[2] = {0.5f / (float) w, 0.5f / (float) h};

        for(unsigned int i = 0; i < 2; i++) {
            pigeon_opengl_bind_framebuffer(&singleton_data.gl.ssao_framebuffers[1 + 1-(i % 2)]);
            pigeon_opengl_bind_texture(0, &singleton_data.ssao_images[!i ? 1 : 2].gltex2d);
            pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_ssao_blur);
            pigeon_opengl_set_uniform_vec2(&singleton_data.gl.shader_ssao_blur,
                singleton_data.gl.shader_ssao_blur_SAMPLE_DISTANCE,
                blur_pushc[0], blur_pushc[1]);
            pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);



            blur_pushc[0] += 1.0f / (float) w;
            blur_pushc[1] += 1.0f / (float) h;
        }

        pigeon_opengl_set_timer_query_value(&objects->gl.timer_queries, 1+PIGEON_WGI_RENDER_STAGE_SSAO);
    }
    else {
        PigeonVulkanCommandPool * p = &objects->command_pools[PIGEON_WGI_RENDER_STAGE_SSAO];
        if(p->recorded) return 0;
        ASSERT_R1(!pigeon_vulkan_start_submission(p, 0));
        if(pigeon_vulkan_general_queue_supports_timestamps())
            pigeon_vulkan_set_timer(p, 0, &objects->timer_query_pool, 2*PIGEON_WGI_RENDER_STAGE_SSAO);

        // SSAO

        float pushc[5] = {
            singleton_data.znear, singleton_data.zfar,
            1 / (float)sc_info.width, 1 / (float)sc_info.height,
            singleton_data.ssao_cutoff
        };

        pigeon_vulkan_start_render_pass(p, 0, &singleton_data.rp_ssao, 
            &singleton_data.ssao_framebuffers[0], sc_info.width, sc_info.height, false);
        pigeon_vulkan_set_viewport_size(p, 0, sc_info.width, sc_info.height);
        pigeon_vulkan_bind_pipeline(p, 0, &singleton_data.pipeline_ssao);
        pigeon_vulkan_bind_descriptor_set(p, 0, &singleton_data.pipeline_ssao, &singleton_data.ssao_descriptor_pools[0], 0);
        pigeon_vulkan_draw(p, 0, 0, 3, 1, &singleton_data.pipeline_ssao, sizeof pushc, pushc);
        pigeon_vulkan_end_render_pass(p, 0);
        pigeon_vulkan_wait_for_colour_write(p, 0, &singleton_data.ssao_images[0].image);
        


        // SSAO 4x downscale

        unsigned int src_w = sc_info.width;
        unsigned int src_h = sc_info.height;
        unsigned int dst_w = sc_info.width / 4;
        unsigned int dst_h = sc_info.height / 4;

        float downscale_pushc[2] = {1 / (float)src_w, 1 / (float)src_h};


        pigeon_vulkan_set_viewport_size(p, 0, dst_w, dst_h);
        pigeon_vulkan_start_render_pass(p, 0, &singleton_data.rp_ssao, 
            &singleton_data.ssao_framebuffers[1], dst_w, dst_h, false);
        pigeon_vulkan_bind_pipeline(p, 0, &singleton_data.pipeline_ssao_downscale_x4);
        pigeon_vulkan_bind_descriptor_set(p, 0, &singleton_data.pipeline_ssao_downscale_x4, 
            &singleton_data.ssao_descriptor_pools[1], 0);
        pigeon_vulkan_draw(p, 0, 0, 3, 1, &singleton_data.pipeline_ssao_downscale_x4, sizeof downscale_pushc, downscale_pushc);
        pigeon_vulkan_end_render_pass(p, 0);
        pigeon_vulkan_wait_for_colour_write(p, 0, &singleton_data.ssao_images[1].image);

        // ssao blur
        
        src_w = dst_w;
        src_h = dst_h;
        float blur_pushc[2] = {0.5f / (float) src_w, 0.5f / (float) src_h};

        for(unsigned int i = 0; i < 2; i++) {
            pigeon_vulkan_start_render_pass(p, 0, &singleton_data.rp_ssao, 
                &singleton_data.ssao_framebuffers[1 + 1-(i % 2)], dst_w, dst_h, false);
            pigeon_vulkan_set_viewport_size(p, 0, dst_w, dst_h);
            pigeon_vulkan_bind_pipeline(p, 0, &singleton_data.pipeline_ssao_blur);
            pigeon_vulkan_bind_descriptor_set(p, 0, &singleton_data.pipeline_ssao_blur,
                &singleton_data.ssao_descriptor_pools[!i ? 2 : 3], 0);
            pigeon_vulkan_draw(p, 0, 0, 3, 1, &singleton_data.pipeline_ssao_blur, sizeof blur_pushc, blur_pushc);
            pigeon_vulkan_end_render_pass(p, 0);

            if(!i)
                pigeon_vulkan_wait_for_colour_write(p, 0, &singleton_data.ssao_images[1 + 1-(i % 2)].image);

            blur_pushc[0] += 1.0f / (float) src_w;
            blur_pushc[1] += 1.0f / (float) src_h;
        }
        if(pigeon_vulkan_general_queue_supports_timestamps())
            pigeon_vulkan_set_timer(p, 0, &objects->timer_query_pool, 1+2*PIGEON_WGI_RENDER_STAGE_SSAO);
        ASSERT_R1(!pigeon_vulkan_end_submission(p, 0));
    }
    return 0;
}

PIGEON_ERR_RET pigeon_wgi_record_bloom(void)
{
    if(!singleton_data.active_render_cfg.bloom) return 0;
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();

    if(OPENGL) {
        pigeon_opengl_set_draw_state(&singleton_data.gl.full_screen_tri_cfg);
            
        // downscale x2 & 1 pass kawase blur

        unsigned int src_w = sc_info.width;
        unsigned int src_h = sc_info.height;
        unsigned int dst_w = src_w / 2;
        unsigned int dst_h = src_h / 2;

        for(unsigned int i = 0; i < 3; i++) {
            float downscale_pushc[3] = {
                1 / (float)src_w, 1 / (float)src_h,
                (2.0f + (float)i*0.5f) / singleton_data.brightness // minimum light
            };

            pigeon_opengl_bind_framebuffer(&singleton_data.gl.bloom_framebuffers[i][0]);
            pigeon_opengl_bind_texture(0, 
                i == 0 ? &singleton_data.render_image.gltex2d : &singleton_data.bloom_images[i-1][1].gltex2d);
            pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_bloom_downscale);

            pigeon_opengl_set_uniform_vec3(&singleton_data.gl.shader_bloom_downscale, 
                singleton_data.gl.shader_bloom_downscale_u_offset_and_min,
                downscale_pushc[0], downscale_pushc[1], downscale_pushc[2]
            );

            pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);

            src_w /= 2;
            src_h /= 2;

            pigeon_opengl_bind_framebuffer(&singleton_data.gl.bloom_framebuffers[i][1]);  
            pigeon_opengl_bind_texture(0, &singleton_data.bloom_images[i][0].gltex2d);
            pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_blur);
            pigeon_opengl_set_uniform_vec2(&singleton_data.gl.shader_blur, 
                singleton_data.gl.shader_blur_SAMPLE_DISTANCE,
                0.5f / (float) src_w, 0.5f / (float) src_h
            );
            pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);

            dst_w /= 2;
            dst_h /= 2;
        }

        // Extra kawase blur


        dst_w *= 2;
        dst_h *= 2;

        pigeon_opengl_bind_framebuffer(&singleton_data.gl.bloom_framebuffers[2][0]);  
        pigeon_opengl_bind_texture(0, &singleton_data.bloom_images[2][1].gltex2d);
        pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_blur);
        pigeon_opengl_set_uniform_vec2(&singleton_data.gl.shader_blur, 
            singleton_data.gl.shader_blur_SAMPLE_DISTANCE,
            1.5f / (float) src_w, 1.5f / (float) src_h
        );
        pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);



        // upscale, kawase, merge

        for(unsigned int i = 0; i < 2; i++) {
            src_w = singleton_data.bloom_images[1-i][1].gltex2d.width;
            src_h = singleton_data.bloom_images[1-i][1].gltex2d.height;

            pigeon_opengl_bind_framebuffer(&singleton_data.gl.bloom_framebuffers[1-i][0]);
            pigeon_opengl_bind_texture(0, &singleton_data.bloom_images[1-i][1].gltex2d);
            pigeon_opengl_bind_texture(1, &singleton_data.bloom_images[(1-i)+1][0].gltex2d);
            pigeon_opengl_set_uniform_vec2(&singleton_data.gl.shader_kawase_merge, 
                singleton_data.gl.shader_kawase_merge_SAMPLE_DISTANCE,
                1.5f / (float)src_w, 1.5f / (float)src_h
            );
            pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);
        }

        pigeon_opengl_set_timer_query_value(&objects->gl.timer_queries, 1+PIGEON_WGI_RENDER_STAGE_BLOOM);

       
    }
    else {
        PigeonVulkanCommandPool * p = &objects->command_pools[PIGEON_WGI_RENDER_STAGE_BLOOM];
        if(p->recorded || !singleton_data.active_render_cfg.bloom) return 0;
        ASSERT_R1(!pigeon_vulkan_start_submission(p, 0));
        if(pigeon_vulkan_general_queue_supports_timestamps())
            pigeon_vulkan_set_timer(p, 0, &objects->timer_query_pool, 2*PIGEON_WGI_RENDER_STAGE_BLOOM);
    
        // downscale x2 & 1 pass kawase blur

        unsigned int src_w = sc_info.width;
        unsigned int src_h = sc_info.height;
        unsigned int dst_w = src_w / 2;
        unsigned int dst_h = src_h / 2;

        for(unsigned int i = 0; i < 3; i++) {
            float downscale_pushc[3] = {
                1 / (float)src_w, 1 / (float)src_h,
                (2.0f + (float)i*0.5f) / singleton_data.brightness // minimum light
            };
            

            pigeon_vulkan_set_viewport_size(p, 0, dst_w, dst_h);
            pigeon_vulkan_start_render_pass(p, 0, &singleton_data.rp_bloom_blur, 
                &singleton_data.bloom_framebuffers[i][0], dst_w, dst_h, false);
            pigeon_vulkan_bind_pipeline(p, 0, &singleton_data.pipeline_downscale_x2);
            pigeon_vulkan_bind_descriptor_set(p, 0, &singleton_data.pipeline_downscale_x2, 
                i == 0 ? &singleton_data.bloom_downscale_descriptor_pool : &singleton_data.bloom_descriptor_pools[i-1][1], 0);
            pigeon_vulkan_draw(p, 0, 0, 3, 1, &singleton_data.pipeline_downscale_x2, sizeof downscale_pushc, downscale_pushc);
            pigeon_vulkan_end_render_pass(p, 0);
            pigeon_vulkan_wait_for_colour_write(p, 0, &singleton_data.bloom_images[i][0].image);

            src_w /= 2;
            src_h /= 2;
            
            float blur_pushc[2] = {0.5f / (float) src_w, 0.5f / (float) src_h};

            pigeon_vulkan_set_viewport_size(p, 0, dst_w, dst_h);
            pigeon_vulkan_start_render_pass(p, 0, &singleton_data.rp_bloom_blur, 
                &singleton_data.bloom_framebuffers[i][1], dst_w, dst_h, false);
            pigeon_vulkan_bind_pipeline(p, 0, &singleton_data.pipeline_blur);
            pigeon_vulkan_bind_descriptor_set(p, 0, &singleton_data.pipeline_blur, &singleton_data.bloom_descriptor_pools[i][0], 0);
            pigeon_vulkan_draw(p, 0, 0, 3, 1, &singleton_data.pipeline_blur, sizeof blur_pushc, blur_pushc);
            pigeon_vulkan_end_render_pass(p, 0);
            pigeon_vulkan_wait_for_colour_write(p, 0, &singleton_data.bloom_images[i][1].image);

            dst_w /= 2;
            dst_h /= 2;
        }

        // Extra kawase blur
        
        dst_w *= 2;
        dst_h *= 2;
            
        float blur_pushc[2] = {1.5f / (float) src_w, 1.5f / (float) src_h};

        pigeon_vulkan_set_viewport_size(p, 0, dst_w, dst_h);
        pigeon_vulkan_start_render_pass(p, 0, &singleton_data.rp_bloom_blur, 
            &singleton_data.bloom_framebuffers[2][0], dst_w, dst_h, false);
        pigeon_vulkan_bind_pipeline(p, 0, &singleton_data.pipeline_blur);
        pigeon_vulkan_bind_descriptor_set(p, 0, &singleton_data.pipeline_blur, &singleton_data.bloom_descriptor_pools[2][1], 0);
        pigeon_vulkan_draw(p, 0, 0, 3, 1, &singleton_data.pipeline_blur, sizeof blur_pushc, blur_pushc);
        pigeon_vulkan_end_render_pass(p, 0);
        pigeon_vulkan_wait_for_colour_write(p, 0, &singleton_data.bloom_images[2][0].image);


        // upscale, kawase, merge

        for(unsigned int i = 0; i < 2; i++) {
            dst_w = src_w = singleton_data.bloom_images[1-i][0].image.width;
            dst_h = src_h = singleton_data.bloom_images[1-i][0].image.height;

            blur_pushc[0] = 1.5f / (float)src_w;
            blur_pushc[1] = 1.5f / (float)src_h;

            pigeon_vulkan_set_viewport_size(p, 0, dst_w, dst_h);
            pigeon_vulkan_start_render_pass(p, 0, &singleton_data.rp_bloom_blur, 
                &singleton_data.bloom_framebuffers[1-i][0], dst_w, dst_h, false);
            pigeon_vulkan_bind_pipeline(p, 0, &singleton_data.pipeline_kawase_merge);
            pigeon_vulkan_bind_descriptor_set(p, 0, &singleton_data.pipeline_kawase_merge, 
                i ? &singleton_data.bloom_blur_merge_descriptor_pool1:
                    &singleton_data.bloom_blur_merge_descriptor_pool0, 0);
            pigeon_vulkan_draw(p, 0, 0, 3, 1, &singleton_data.pipeline_kawase_merge, sizeof blur_pushc, blur_pushc);
            pigeon_vulkan_end_render_pass(p, 0);
            pigeon_vulkan_wait_for_colour_write(p, 0, &singleton_data.bloom_images[1-i][0].image); 
        }
        if(pigeon_vulkan_general_queue_supports_timestamps())
            pigeon_vulkan_set_timer(p, 0, &objects->timer_query_pool, 1+2*PIGEON_WGI_RENDER_STAGE_BLOOM);
        ASSERT_R1(!pigeon_vulkan_end_submission(p, 0));
        
    }
    return 0;
}




PIGEON_ERR_RET pigeon_wgi_submit_frame(void)
{
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    if(OPENGL) {
        objects->first_frame_submitted = true; 
        pigeon_wgi_swap_buffers();

        singleton_data.previous_frame_index_mod = singleton_data.current_frame_index_mod;
        singleton_data.current_frame_index_mod = (singleton_data.current_frame_index_mod+1) % singleton_data.frame_objects_count;

        return 0;
    }


    const bool first_frame = singleton_data.previous_frame_index_mod == UINT32_MAX;
    PerFrameData * prev_objects = first_frame ? NULL : &singleton_data.per_frame_objects[singleton_data.previous_frame_index_mod];


    objects->first_frame_submitted = true;    


    ASSERT_R1(
        objects->command_pools[PIGEON_WGI_RENDER_STAGE_UPLOAD].recorded &&
        objects->command_pools[PIGEON_WGI_RENDER_STAGE_DEPTH].recorded &&
        objects->command_pools[PIGEON_WGI_RENDER_STAGE_RENDER].recorded &&
        objects->command_pools[PIGEON_WGI_RENDER_STAGE_POST_AND_UI].recorded
    );
    if(singleton_data.active_render_cfg.ssao) ASSERT_R1(objects->command_pools[PIGEON_WGI_RENDER_STAGE_SSAO].recorded);
    if(singleton_data.active_render_cfg.bloom) ASSERT_R1(objects->command_pools[PIGEON_WGI_RENDER_STAGE_BLOOM].recorded);

    unsigned int shadows_count = 0;
    for(unsigned int j = 0; j < 4; j++) {
        if(!singleton_data.shadow_parameters[j].resolution) continue;

        shadows_count++;
        ASSERT_R1(objects->command_pools[PIGEON_WGI_RENDER_STAGE_SHADOW0+j].recorded);
    }


    PigeonVulkanSubmitInfo submissions[PIGEON_WGI_RENDER_STAGE__COUNT] = {{0}};

    // upload
    submissions[0].pool = &objects->command_pools[PIGEON_WGI_RENDER_STAGE_UPLOAD];

    if(!first_frame) {
        submissions[0].wait_semaphore_count = 1;
        submissions[0].wait_semaphores[0] = prev_objects->semaphores.render_done[0];
        submissions[0].wait_semaphore_types[0] = PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_ALL;
    }

    submissions[0].signal_semaphores[0] = objects->semaphores.upload_done[0];
    submissions[0].signal_semaphores[1] = objects->semaphores.upload_done[1];

    unsigned int signal_idx = 2;
    for(unsigned int j = 0; j < 4; j++) {
        if(!singleton_data.shadow_parameters[j].resolution) continue;

        submissions[0].signal_semaphores[signal_idx++] = objects->semaphores.upload_done[2+j];
    }
    submissions[0].signal_semaphore_count = signal_idx;


    // shadows

    unsigned int i = 1;
    for(unsigned int j = 0; j < 4; j++) {
        if(!singleton_data.shadow_parameters[j].resolution) continue;

        submissions[i].pool = &objects->command_pools[PIGEON_WGI_RENDER_STAGE_SHADOW0+j];
        submissions[i].wait_semaphore_count = 1;
        submissions[i].wait_semaphore_types[0] = PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_ALL;
        submissions[i].wait_semaphores[0] = objects->semaphores.upload_done[2+j];

        submissions[i].signal_semaphore_count = 1;
        submissions[i].signal_semaphores[0] = objects->semaphores.shadows_done[j];

        i++;
    }

    // depth
    submissions[i].pool = &objects->command_pools[PIGEON_WGI_RENDER_STAGE_DEPTH];
    submissions[i].wait_semaphore_count = 1;
    submissions[i].wait_semaphore_types[0] = PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_ALL;
    submissions[i].wait_semaphores[0] = objects->semaphores.upload_done[0];
    submissions[i].signal_semaphore_count = 1;
    submissions[i].signal_semaphores[0] = objects->semaphores.depth_done;
    i++;

    // ssao
    if(singleton_data.active_render_cfg.ssao) {
        submissions[i].pool = &objects->command_pools[PIGEON_WGI_RENDER_STAGE_SSAO];
        submissions[i].wait_semaphore_count = 1;
        submissions[i].wait_semaphore_types[0] = PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_FRAGMENT;
        submissions[i].wait_semaphores[0] = objects->semaphores.depth_done;
        submissions[i].signal_semaphore_count = 1;
        submissions[i].signal_semaphores[0] = objects->semaphores.ssao_done;
        i++;
    }

    // render
    submissions[i].pool = &objects->command_pools[PIGEON_WGI_RENDER_STAGE_RENDER];
    
    submissions[i].wait_semaphore_types[0] = PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_ALL;
    submissions[i].wait_semaphores[0] = objects->semaphores.upload_done[1];

    submissions[i].wait_semaphore_types[1] = PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_FRAGMENT;
    submissions[i].wait_semaphores[1] = singleton_data.active_render_cfg.ssao ?
        objects->semaphores.ssao_done : objects->semaphores.depth_done;

    submissions[i].wait_semaphore_count = 2;
    for(unsigned int j = 0; j < 4; j++) {
        if(!singleton_data.shadow_parameters[j].resolution) continue;

        submissions[i].wait_semaphore_types[submissions[i].wait_semaphore_count] = 
            PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_FRAGMENT;
        submissions[i].wait_semaphores[submissions[i].wait_semaphore_count++] = objects->semaphores.shadows_done[j];
    }

    if(!first_frame) {
        submissions[i].wait_semaphore_types[submissions[i].wait_semaphore_count] = 
            PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_COLOUR_WRITE;
        submissions[i].wait_semaphores[submissions[i].wait_semaphore_count++] = prev_objects->semaphores.post_done[0];
    }

    submissions[i].signal_semaphore_count = 2;
    submissions[i].signal_semaphores[0] = objects->semaphores.render_done[0];
    submissions[i].signal_semaphores[1] = objects->semaphores.render_done[1];
    i++;

    // bloom
    if(singleton_data.active_render_cfg.bloom) {
        submissions[i].pool = &objects->command_pools[PIGEON_WGI_RENDER_STAGE_BLOOM];
        submissions[i].wait_semaphore_count = 1;
        submissions[i].wait_semaphore_types[0] = PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_FRAGMENT;
        submissions[i].wait_semaphores[0] = objects->semaphores.render_done[1];
        submissions[i].signal_semaphore_count = 1;
        submissions[i].signal_semaphores[0] = objects->semaphores.bloom_done;
        i++;
    }

    // post+ui
    submissions[i].pool = &objects->command_pools[PIGEON_WGI_RENDER_STAGE_POST_AND_UI];
    submissions[i].wait_semaphore_count = 2;
    submissions[i].wait_semaphore_types[0] = PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_FRAGMENT;
    submissions[i].wait_semaphores[0] = singleton_data.active_render_cfg.bloom ?
        objects->semaphores.bloom_done : objects->semaphores.render_done[1];
    submissions[i].wait_semaphore_types[1] = PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_COLOUR_WRITE;
    submissions[i].wait_semaphores[1] = objects->semaphores.swapchain_aquisition;

    submissions[i].signal_semaphore_count = 2;
    submissions[i].signal_semaphores[0] = objects->semaphores.post_done[0];
    submissions[i].signal_semaphores[1] = objects->semaphores.post_done[1];
    
    i++;

    ASSERT_R1(!pigeon_vulkan_submit_multi(submissions, i, &objects->render_done_fence));




    singleton_data.previous_frame_index_mod = singleton_data.current_frame_index_mod;
    singleton_data.current_frame_index_mod = (singleton_data.current_frame_index_mod+1) % singleton_data.frame_objects_count;

    /* Swapchain */

    int swapchain_err = pigeon_vulkan_swapchain_present(&objects->semaphores.post_done[1]);
    if(swapchain_err == 2) {
        ASSERT_R1(!recreate_swapchain_loop());
        swapchain_err = 0;
    }

    return swapchain_err;
}


