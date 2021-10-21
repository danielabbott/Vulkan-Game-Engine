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
#include "tex.h"

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
            ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->primary_command_pool, 3, true, false));
        }

        objects->upload_command_buffer.no_render = true;
        objects->depth_command_buffer.depth_only = true;
        objects->light_pass_command_buffer.light_pass = true;
        objects->shadow_command_buffers[0].depth_only = true;
        objects->shadow_command_buffers[1].depth_only = true;
        objects->shadow_command_buffers[2].depth_only = true;
        objects->shadow_command_buffers[3].depth_only = true;
        objects->shadow_command_buffers[0].shadow = true;
        objects->shadow_command_buffers[1].shadow = true;
        objects->shadow_command_buffers[2].shadow = true;
        objects->shadow_command_buffers[3].shadow = true;
        objects->shadow_command_buffers[0].mvp_index = 1;
        objects->shadow_command_buffers[1].mvp_index = 2;
        objects->shadow_command_buffers[2].mvp_index = 3;
        objects->shadow_command_buffers[3].mvp_index = 4;

        
        // Uniform buffers are (re)created as needed and the descriptors are set there too


        if(VULKAN) {
            ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->depth_command_buffer.command_pool, 1, false, false));
            ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->shadow_command_buffers[0].command_pool, 1, false, false));
            ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->shadow_command_buffers[1].command_pool, 1, false, false));
            ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->shadow_command_buffers[2].command_pool, 1, false, false));
            ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->shadow_command_buffers[3].command_pool, 1, false, false));
            ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->upload_command_buffer.command_pool, 1, false, false));
            ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->light_pass_command_buffer.command_pool, 1, false, false));
            ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->render_command_buffer.command_pool, 1, false, false));

            ASSERT_R1(!pigeon_vulkan_create_descriptor_pool(&objects->depth_descriptor_pool, 1, &singleton_data.depth_descriptor_layout));
            ASSERT_R1(!pigeon_vulkan_create_descriptor_pool(&objects->light_pass_descriptor_pool, 1, &singleton_data.render_descriptor_layout));
            ASSERT_R1(!pigeon_vulkan_create_descriptor_pool(&objects->render_descriptor_pool, 1, &singleton_data.render_descriptor_layout));

            pigeon_vulkan_set_descriptor_texture(&objects->light_pass_descriptor_pool, 0, 3, 0, 
                &singleton_data.depth_image.image_view, &singleton_data.nearest_filter_sampler);

            for(unsigned int j = 0; j < 4; j++) {
                pigeon_vulkan_set_descriptor_texture(&objects->light_pass_descriptor_pool, 0, 4, j, 
                    &singleton_data.default_shadow_map_image_view, &singleton_data.shadow_sampler);
                pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 4, j, 
                    &singleton_data.default_shadow_map_image_view, &singleton_data.shadow_sampler);
            }


            pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 3, 0, 
                &singleton_data.light_image.image_view, &singleton_data.bilinear_sampler);


            // TODO do in bulk
            for(unsigned int j = 0; j < 59; j++) {
                pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 5, j, 
                    &singleton_data.default_1px_white_texture_array_image_view, &singleton_data.texture_sampler);
                pigeon_vulkan_set_descriptor_texture(&objects->light_pass_descriptor_pool, 0, 5, j, 
                    &singleton_data.default_1px_white_texture_array_image_view, &singleton_data.texture_sampler);
            }

            ASSERT_R1(!pigeon_vulkan_create_fence(&objects->pre_render_done_fence, false));

            ASSERT_R1(!pigeon_vulkan_create_semaphore(&objects->pre_processing_done_semaphore));
            ASSERT_R1(!pigeon_vulkan_create_semaphore(&objects->render_done_semaphore));
            ASSERT_R1(!pigeon_vulkan_create_semaphore(&objects->post_processing_done_semaphore));
            ASSERT_R1(!pigeon_vulkan_create_semaphore(&objects->render_done_semaphore2));
            ASSERT_R1(!pigeon_vulkan_create_semaphore(&objects->post_processing_done_semaphore2));
            ASSERT_R1(!pigeon_vulkan_create_semaphore(&objects->swapchain_acquire_semaphore));
            

            if(pigeon_vulkan_general_queue_supports_timestamps())
                ASSERT_R1(!pigeon_vulkan_create_timer_query_pool(&objects->timer_query_pool, PIGEON_WGI_TIMERS_COUNT));
        

            // Framebuffer is specific to swapchain image - NOT current frame index mod

            ASSERT_R1(!pigeon_vulkan_create_framebuffer(&singleton_data.post_framebuffers[i],
                NULL, pigeon_vulkan_get_swapchain_image_view(i), &singleton_data.rp_post));
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

            if(objects->pre_processing_done_semaphore.vk_semaphore) 
                pigeon_vulkan_destroy_semaphore(&objects->pre_processing_done_semaphore);
            if(objects->render_done_semaphore.vk_semaphore) 
                pigeon_vulkan_destroy_semaphore(&objects->render_done_semaphore);
            if(objects->post_processing_done_semaphore.vk_semaphore) 
                pigeon_vulkan_destroy_semaphore(&objects->post_processing_done_semaphore);

            if(objects->render_done_semaphore2.vk_semaphore) 
                pigeon_vulkan_destroy_semaphore(&objects->render_done_semaphore2);
            if(objects->post_processing_done_semaphore2.vk_semaphore) 
                pigeon_vulkan_destroy_semaphore(&objects->post_processing_done_semaphore2);

            if(objects->swapchain_acquire_semaphore.vk_semaphore) 
                pigeon_vulkan_destroy_semaphore(&objects->swapchain_acquire_semaphore);

                

            if(objects->pre_render_done_fence.vk_fence) pigeon_vulkan_destroy_fence(&objects->pre_render_done_fence);

            if(objects->depth_descriptor_pool.vk_descriptor_pool)
                pigeon_vulkan_destroy_descriptor_pool(&objects->depth_descriptor_pool);
            if(objects->render_descriptor_pool.vk_descriptor_pool)
                pigeon_vulkan_destroy_descriptor_pool(&objects->render_descriptor_pool);
            if(objects->light_pass_descriptor_pool.vk_descriptor_pool)
                pigeon_vulkan_destroy_descriptor_pool(&objects->light_pass_descriptor_pool);

            if(objects->light_pass_command_buffer.command_pool.vk_command_pool)
                pigeon_vulkan_destroy_command_pool(&objects->light_pass_command_buffer.command_pool);
            if(objects->render_command_buffer.command_pool.vk_command_pool)
                pigeon_vulkan_destroy_command_pool(&objects->render_command_buffer.command_pool);
            if(objects->depth_command_buffer.command_pool.vk_command_pool)
                pigeon_vulkan_destroy_command_pool(&objects->depth_command_buffer.command_pool);

            for(unsigned int j = 0; j < 4; j++)
                if(objects->shadow_command_buffers[j].command_pool.vk_command_pool)
                    pigeon_vulkan_destroy_command_pool(&objects->shadow_command_buffers[j].command_pool);

            if(objects->upload_command_buffer.command_pool.vk_command_pool)
                pigeon_vulkan_destroy_command_pool(&objects->upload_command_buffer.command_pool);
            if(objects->primary_command_pool.vk_command_pool)
                pigeon_vulkan_destroy_command_pool(&objects->primary_command_pool);

            if(objects->uniform_buffer.vk_buffer) pigeon_vulkan_destroy_buffer(&objects->uniform_buffer);
            if(objects->uniform_buffer_memory.vk_device_memory)
                pigeon_vulkan_free_memory(&objects->uniform_buffer_memory);
        }
    }

    if(OPENGL) {
        for(unsigned int i = 0; i < singleton_data.frame_objects_count; i++) {
            PerFrameData * objects = &singleton_data.per_frame_objects[i];

            pigeon_opengl_destroy_buffer(&objects->gl.uniform_buffer);
        }
    }

    free2((void**)&singleton_data.per_frame_objects);
    free2((void**)&singleton_data.post_framebuffers);

    singleton_data.per_frame_objects = NULL;
    singleton_data.post_framebuffers = NULL;
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

static PIGEON_ERR_RET reset_buffers()
{    
    if(VULKAN)
        ASSERT_R1(!pigeon_vulkan_reset_command_pool(
            &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod].primary_command_pool));
   
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
            pigeon_vulkan_set_descriptor_uniform_buffer2(&objects->light_pass_descriptor_pool, 0, 0, 0,
                &objects->uniform_buffer, uniform_offset, sizeof(PigeonWGISceneUniformData));

            uniform_offset += round_up(sizeof(PigeonWGISceneUniformData), align);
                
            pigeon_vulkan_set_descriptor_ssbo2(&objects->render_descriptor_pool, 0, 1, 0,
                &objects->uniform_buffer, uniform_offset,
                sizeof(PigeonWGIDrawObject) * singleton_data.max_draws);
            pigeon_vulkan_set_descriptor_ssbo2(&objects->depth_descriptor_pool, 0, 1, 0,
                &objects->uniform_buffer, uniform_offset,
                sizeof(PigeonWGIDrawObject) * singleton_data.max_draws);
            pigeon_vulkan_set_descriptor_ssbo2(&objects->light_pass_descriptor_pool, 0, 1, 0,
                &objects->uniform_buffer, uniform_offset,
                sizeof(PigeonWGIDrawObject) * singleton_data.max_draws);
                
            uniform_offset += round_up(sizeof(PigeonWGIDrawObject) * singleton_data.max_draws, align);
            uniform_offset += round_up(sizeof(PigeonVulkanDrawIndexedIndirectCommand) * singleton_data.max_multidraw_draws, align);
                
            pigeon_vulkan_set_descriptor_ssbo2(&objects->render_descriptor_pool, 0, 2, 0,
                &objects->uniform_buffer, uniform_offset, singleton_data.total_bones * sizeof(PigeonWGIBoneMatrix));
            pigeon_vulkan_set_descriptor_ssbo2(&objects->depth_descriptor_pool, 0, 2, 0,
                &objects->uniform_buffer, uniform_offset, singleton_data.total_bones * sizeof(PigeonWGIBoneMatrix));
            pigeon_vulkan_set_descriptor_ssbo2(&objects->light_pass_descriptor_pool, 0, 2, 0,
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


PIGEON_ERR_RET pigeon_wgi_next_frame_wait(double delayed_timer_values[PIGEON_WGI_TIMERS_COUNT])
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

PIGEON_ERR_RET pigeon_wgi_next_frame_poll(double delayed_timer_values[PIGEON_WGI_TIMERS_COUNT], bool* ready)
{
    pigeon_wgi_poll_events();
    bool block = ready == NULL;
        
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    if(VULKAN) {
        PerFrameData * prev_objects = NULL;
        if(singleton_data.previous_frame_index_mod != UINT32_MAX)
            prev_objects = &singleton_data.per_frame_objects[singleton_data.previous_frame_index_mod];


        if(prev_objects && prev_objects->commands_in_progress) {
            if(block) {
                ASSERT_R1(!pigeon_vulkan_wait_fence(&prev_objects->pre_render_done_fence));
                ASSERT_R1(!pigeon_vulkan_reset_fence(&prev_objects->pre_render_done_fence));
                prev_objects->commands_in_progress = false;
            }
            else {
                bool fence_state;
                ASSERT_R1(!pigeon_vulkan_poll_fence(&prev_objects->pre_render_done_fence, &fence_state));

                if(fence_state) {
                    ASSERT_R1(!pigeon_vulkan_reset_fence(&prev_objects->pre_render_done_fence));
                    prev_objects->commands_in_progress = false;
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
        memset(delayed_timer_values, 0, sizeof(double) * PIGEON_WGI_TIMERS_COUNT);


        return 0;
    }





    int swapchain_err = pigeon_vulkan_next_swapchain_image(&singleton_data.swapchain_image_index, 
        &objects->swapchain_acquire_semaphore, NULL, block);

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
        if(pigeon_vulkan_general_queue_supports_timestamps() && (!objects->first_frame_submitted ||
            pigeon_vulkan_get_timer_results(&objects->timer_query_pool, delayed_timer_values)))
        {        
            memset(delayed_timer_values, 0, sizeof(double) * PIGEON_WGI_TIMERS_COUNT);
        }
    }

    return 0;
    
}

unsigned int pigeon_wgi_get_bone_data_alignment(void)
{
    if(VULKAN) return 1;
    else return (pigeon_opengl_get_uniform_buffer_min_alignment() + 4*3*4 - 1) / (4*3*4);
}

PIGEON_ERR_RET pigeon_wgi_start_frame(unsigned int max_draws,
    uint32_t max_multidraw_draws,
    PigeonWGIShadowParameters shadows[4], unsigned int total_bones,
    PigeonWGIBoneMatrix ** bone_matrices)
{

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

    singleton_data.multidraw_draw_index = 0;

    
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod]; 


    memcpy(singleton_data.shadow_parameters, shadows, sizeof singleton_data.shadow_parameters);
    ASSERT_R1(!pigeon_wgi_assign_shadow_framebuffers());

    ASSERT_R1(!reset_buffers());
    objects->commands_in_progress = true;
    ASSERT_R1(!prepare_uniform_buffers());


    // Bind shadow textures

    if(VULKAN) {
        for(unsigned int i = 0; i < 4; i++) {
            PigeonWGIShadowParameters* p = &singleton_data.shadow_parameters[i];
            if(p->resolution) {
                unsigned int j = (unsigned)p->framebuffer_index;
                pigeon_vulkan_set_descriptor_texture(&objects->light_pass_descriptor_pool, 0, 4, i, 
                    &singleton_data.shadow_images[j].image_view, &singleton_data.shadow_sampler);
                pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 4, i, 
                    &singleton_data.shadow_images[j].image_view, &singleton_data.shadow_sampler);
            }
            else {
                pigeon_vulkan_set_descriptor_texture(&objects->light_pass_descriptor_pool, 0, 4, i, 
                    &singleton_data.default_shadow_map_image_view, &singleton_data.shadow_sampler);
                pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 4, i, 
                    &singleton_data.default_shadow_map_image_view, &singleton_data.shadow_sampler);
            }
        }

        const unsigned int align = pigeon_vulkan_get_buffer_min_alignment();

        uint8_t * dst = objects->uniform_buffer_memory.mapping;
        dst += round_up(sizeof(PigeonWGISceneUniformData), align);
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
        dst += round_up(sizeof(PigeonWGIDrawObject), align) * singleton_data.max_draws;

        *bone_matrices = (PigeonWGIBoneMatrix *) (void *) dst;
    }    

    return 0;
}

PIGEON_ERR_RET pigeon_wgi_set_uniform_data(PigeonWGISceneUniformData * uniform_data,
    PigeonWGIDrawObject * draw_objects, unsigned int draws_count)
{
    if(draws_count) ASSERT_R1(draw_objects)

    uniform_data->znear = singleton_data.znear;
    uniform_data->zfar = singleton_data.zfar;
    pigeon_wgi_set_shadow_uniforms(uniform_data, draw_objects, draws_count);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    assert(draws_count <= singleton_data.max_draws);



    const unsigned int align = VULKAN ? pigeon_vulkan_get_buffer_min_alignment()
        : pigeon_opengl_get_uniform_buffer_min_alignment();

    uint8_t * dst = VULKAN ? objects->uniform_buffer_memory.mapping : 
        objects->gl.uniform_buffer.mapping;

    memcpy(dst, uniform_data, sizeof(PigeonWGISceneUniformData));
    dst += round_up(sizeof(PigeonWGISceneUniformData), align);


    if(VULKAN) {
        memcpy(dst, draw_objects, draws_count * sizeof(PigeonWGIDrawObject));
        dst += round_up(sizeof(PigeonWGIDrawObject) * singleton_data.max_draws, align);
        dst += round_up(sizeof(PigeonVulkanDrawIndexedIndirectCommand) * singleton_data.max_multidraw_draws, align);

        ASSERT_R1(!pigeon_vulkan_flush_memory(&objects->uniform_buffer_memory, 0, 0));
    }
    else {
        unsigned int sz = round_up(sizeof(PigeonWGIDrawObject), align);
        for(unsigned int i = 0; i < draws_count; i++) {
            *(PigeonWGIDrawObject*)(uintptr_t)dst = draw_objects[i];
            dst += sz;
        }

    
        pigeon_opengl_bind_uniform_buffer(&objects->gl.uniform_buffer);
        pigeon_opengl_unmap_buffer(&objects->gl.uniform_buffer);
    }
    

    return 0;
}

PigeonWGICommandBuffer * pigeon_wgi_get_upload_command_buffer(void)
{
    return &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod].upload_command_buffer;
}

PigeonWGICommandBuffer * pigeon_wgi_get_depth_command_buffer(void)
{
    return &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod].depth_command_buffer;
}

PigeonWGICommandBuffer * pigeon_wgi_get_shadow_command_buffer(unsigned int light_index)
{
    ASSERT_R0(light_index < 4);
    return &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod].shadow_command_buffers[light_index];
}

PigeonWGICommandBuffer * pigeon_wgi_get_light_pass_command_buffer(void)
{
    return &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod].light_pass_command_buffer;
}

PigeonWGICommandBuffer * pigeon_wgi_get_render_command_buffer(void)
{
    return &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod].render_command_buffer;
}

static PIGEON_ERR_RET pigeon_wgi_start_command_buffer_gl(PigeonWGICommandBuffer * command_buffer)
{
    command_buffer->has_been_recorded = true;

    if(command_buffer->no_render) {
        return 0;
    }

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    if(command_buffer == &objects->depth_command_buffer) {
        pigeon_opengl_bind_framebuffer(&singleton_data.gl.depth_framebuffer);
        pigeon_opengl_clear_depth();
    }
    else if(command_buffer >= &objects->shadow_command_buffers[0] && 
            command_buffer < &objects->shadow_command_buffers[0]+4)
    {
        unsigned int j = (unsigned)singleton_data.shadow_parameters[command_buffer->mvp_index-1].framebuffer_index;
        pigeon_opengl_bind_framebuffer(&singleton_data.gl.shadow_framebuffers[j]);
        pigeon_opengl_clear_depth();
    }
    else if(command_buffer == &objects->render_command_buffer) {
        pigeon_opengl_bind_framebuffer(&singleton_data.gl.render_framebuffer);
        pigeon_opengl_bind_texture(0, &singleton_data.shadow_images[0].gltex2d);  
        if(singleton_data.shadow_images[1].gltex2d.id)
            pigeon_opengl_bind_texture(1, &singleton_data.shadow_images[1].gltex2d);  
        pigeon_opengl_bind_texture(5, &singleton_data.light_image.gltex2d);        
    }
    else {
        pigeon_opengl_bind_framebuffer(&singleton_data.gl.light_framebuffer);
        pigeon_opengl_bind_texture(5, &singleton_data.depth_image.gltex2d);
    }


    return 0;
}

PIGEON_ERR_RET pigeon_wgi_start_command_buffer(PigeonWGICommandBuffer * command_buffer)
{
    if(OPENGL) return pigeon_wgi_start_command_buffer_gl(command_buffer);

    ASSERT_R1(!pigeon_vulkan_reset_command_pool(&command_buffer->command_pool));

    if(command_buffer->no_render) {
        ASSERT_R1(!pigeon_vulkan_start_submission2( &command_buffer->command_pool, 0, NULL, NULL));
    }
    else {
        PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
        PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();

        PigeonVulkanRenderPass * rp = NULL;
        PigeonVulkanFramebuffer * fb = NULL;
        unsigned int vp_w = 0, vp_h = 0;


        if(command_buffer == &objects->depth_command_buffer) {
            rp = &singleton_data.rp_depth;
            fb = &singleton_data.depth_framebuffer;
            vp_w = sc_info.width;
            vp_h = sc_info.height;
        }
        else if(command_buffer == &objects->render_command_buffer) {
            rp = &singleton_data.rp_render;
            fb = &singleton_data.render_framebuffer;
            vp_w = sc_info.width;
            vp_h = sc_info.height;
        }
        else if(command_buffer >= &objects->shadow_command_buffers[0] && 
                command_buffer < &objects->shadow_command_buffers[0]+4)
        {
            ASSERT_R1(command_buffer->mvp_index > 0 && command_buffer->mvp_index < 5);

            rp = &singleton_data.rp_depth;
            unsigned int j = (unsigned)singleton_data.shadow_parameters[command_buffer->mvp_index-1].framebuffer_index;
            fb = &singleton_data.shadow_framebuffers[j];
            vp_w = singleton_data.shadow_images[j].image.width;
            vp_h = singleton_data.shadow_images[j].image.height;
        }
        else {
            ASSERT_R1(command_buffer == &objects->light_pass_command_buffer);
            ASSERT_R1(!command_buffer->mvp_index);

            rp = &singleton_data.rp_light_pass;
            fb = &singleton_data.light_framebuffer;
            vp_w = sc_info.width;
            vp_h = sc_info.height;
        }

        ASSERT_R1(!pigeon_vulkan_start_submission2(&command_buffer->command_pool, 0, rp, fb));

        pigeon_vulkan_set_viewport_size(&command_buffer->command_pool, 0, vp_w, vp_h);
    }  

    command_buffer->has_been_recorded = true;
    return 0;
}

typedef void* PipelineOrProgram;

static PipelineOrProgram get_pipeline(PigeonWGICommandBuffer* command_buffer, PigeonWGIPipeline* pipeline)
{
    if(VULKAN) {
        return command_buffer->depth_only ?
            (command_buffer->shadow ? pipeline->pipeline_shadow_map : pipeline->pipeline_depth)
                : (command_buffer->light_pass ? pipeline->pipeline_light : pipeline->pipeline);
    }
    else {
        return command_buffer->depth_only ?
            pipeline->gl.program_depth
                : (command_buffer->light_pass ? pipeline->gl.program_light : pipeline->gl.program);
    }
}

void pigeon_wgi_draw_without_mesh(PigeonWGICommandBuffer* command_buffer, PigeonWGIPipeline* pipeline, 
    unsigned int vertices)
{
    assert(command_buffer && pipeline && !command_buffer->depth_only);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    if(VULKAN) {
        pigeon_vulkan_bind_pipeline(&command_buffer->command_pool, 0, pipeline->pipeline);
        pigeon_vulkan_bind_descriptor_set(&command_buffer->command_pool, 0, pipeline->pipeline, 
            &objects->render_descriptor_pool, 0);

        pigeon_vulkan_draw(&command_buffer->command_pool, 0, 0, vertices, 1,
            get_pipeline(command_buffer, pipeline), 0, NULL);
    }
    else {
        pigeon_opengl_bind_shader_program(get_pipeline(command_buffer, pipeline));
        pigeon_opengl_set_draw_state(&pipeline->gl.config);

        pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);
    }
}

static void draw_setup_common_gl(PigeonWGICommandBuffer* command_buffer, PigeonWGIPipeline* pipeline, 
    uint32_t draw_index, int diffuse_texture, int nmap_texture, 
    unsigned int first_bone_index, unsigned int bones_count)
{
    // Bind shader, set mvp index uniform

    PigeonOpenGLShaderProgram * program = NULL;
    int location = -1;
    int * value_cache = NULL;
    PigeonWGIPipelineConfig const* cfg;
    if(command_buffer->depth_only) {
        program = pipeline->gl.program_depth;
        location = pipeline->gl.program_depth_mvp_loc;
        value_cache = &pipeline->gl.program_depth_mvp;
        cfg = &pipeline->gl.config_depth;

        if(command_buffer->shadow) {
            cfg = &pipeline->gl.config_shadow;
        }
    }
    else {
        if(command_buffer->light_pass) {
            program = pipeline->gl.program_light;
            location = pipeline->gl.program_light_mvp_loc;
            value_cache = &pipeline->gl.program_light_mvp;
            cfg = &pipeline->gl.config_light;
        }
        else {
            program = pipeline->gl.program;
            location = pipeline->gl.program_mvp_loc;
            value_cache = &pipeline->gl.program_mvp;
            cfg = &pipeline->gl.config;
        }
    }

    pigeon_opengl_bind_shader_program(program);
    if(*value_cache != (int) command_buffer->mvp_index) {
        pigeon_opengl_set_uniform_i(program, location, (int) command_buffer->mvp_index);
        *value_cache = (int) command_buffer->mvp_index;
    }
    pigeon_opengl_set_draw_state(cfg);

    // Bind draw data

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    const unsigned int align = pigeon_opengl_get_uniform_buffer_min_alignment();

    pigeon_opengl_bind_uniform_buffer2(&objects->gl.uniform_buffer, 0, 0, 
        sizeof(PigeonWGISceneUniformData)); // TODO don't keep rebinding

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

static void draw_setup_common(PigeonWGICommandBuffer* command_buffer, PigeonWGIPipeline* pipeline, 
    PipelineOrProgram * vpipeline, PigeonWGIMultiMesh* mesh, uint32_t draw_index, 
    int diffuse_texture, int nmap_texture, unsigned int first_bone_index, unsigned int bones_count)
{
    if(OPENGL) { 
        *vpipeline = NULL;
        draw_setup_common_gl(command_buffer, pipeline, draw_index, diffuse_texture, nmap_texture, first_bone_index, bones_count); 
        return; 
    }

    *vpipeline = get_pipeline(command_buffer, pipeline);

    pigeon_vulkan_bind_pipeline(&command_buffer->command_pool, 0, *vpipeline);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    
    pigeon_vulkan_bind_descriptor_set(&command_buffer->command_pool, 0, *vpipeline, 
        command_buffer->depth_only ?
        (pipeline->transparent ? &objects->render_descriptor_pool : &objects->depth_descriptor_pool) : 
        (command_buffer->light_pass ? &objects->light_pass_descriptor_pool : &objects->render_descriptor_pool), 0);

    unsigned int attribute_count = 0;
    for (; attribute_count < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES; attribute_count++) {
        if (!mesh->attribute_types[attribute_count]) break;
    }

    pigeon_vulkan_bind_vertex_buffers(&command_buffer->command_pool, 0, &mesh->staged_buffer->buffer,
        attribute_count, mesh->attribute_start_offsets);
        
    if(mesh->index_count) {
        pigeon_vulkan_bind_index_buffer(&command_buffer->command_pool, 0, 
            &mesh->staged_buffer->buffer, mesh->index_data_offset, mesh->big_indices);
    }
}

void pigeon_wgi_draw(PigeonWGICommandBuffer* command_buffer, PigeonWGIPipeline* pipeline, 
    PigeonWGIMultiMesh* mesh, uint32_t start_vertex,
    uint32_t draw_index, uint32_t instances, uint32_t first, uint32_t count,
    int diffuse_texture, int nmap_texture, unsigned int first_bone_index, unsigned int bones_count)
{
    assert(command_buffer && pipeline && mesh);
    if(VULKAN) assert(pipeline->pipeline && mesh->staged_buffer);
    if(OPENGL) assert(pipeline->gl.program && mesh->opengl_vao);

    if(!instances) instances = 1;
    if(!count) return;

    if(OPENGL && instances > 1) {
        assert(false);
        return;
    }

    PipelineOrProgram vpipeline;
    draw_setup_common(command_buffer, pipeline, &vpipeline, mesh, draw_index, diffuse_texture, nmap_texture, first_bone_index, bones_count);

    uint32_t pushc[2] = {draw_index, command_buffer->mvp_index};
    
    if(mesh->index_count) {
        if(count == UINT32_MAX) {
            count = mesh->index_count - first;
        }

        if(VULKAN) pigeon_vulkan_draw_indexed(&command_buffer->command_pool, 0, 
            start_vertex, first, count, instances, vpipeline, sizeof pushc, &pushc);
        else pigeon_opengl_draw_indexed(mesh->opengl_vao, start_vertex, first, count);
    }
    else {
        if(count == UINT32_MAX) {
            count = mesh->vertex_count - first;
        }

        assert(!start_vertex);

        if(VULKAN) pigeon_vulkan_draw(&command_buffer->command_pool, 0, 
            first, count, instances, vpipeline, sizeof pushc, &pushc);
        else pigeon_opengl_draw(mesh->opengl_vao, first, count);
    }
}


bool pigeon_wgi_multidraw_supported(void)
{
    return VULKAN;
}

void pigeon_wgi_multidraw_draw(unsigned int start_vertex,
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

    PigeonVulkanDrawIndexedIndirectCommand * cmd = &indirect_cmds[singleton_data.multidraw_draw_index++];
    assert(singleton_data.multidraw_draw_index <= singleton_data.max_multidraw_draws);

    
    cmd->indexCount = count;
    cmd->instanceCount = instances;
    cmd->firstIndex = first;
    cmd->vertexOffset = (int32_t)start_vertex;
    cmd->firstInstance = first_instance;
}

void pigeon_wgi_multidraw_submit(PigeonWGICommandBuffer* command_buffer,
    PigeonWGIPipeline* pipeline, PigeonWGIMultiMesh* mesh,
    uint32_t first_multidraw_index, uint32_t multidraw_count, uint32_t first_draw_index, uint32_t draws)
{
    if(!draws) return;

    if(!VULKAN) {
        assert(false);
        return;
    }

    assert(command_buffer && pipeline && pipeline->pipeline && mesh && mesh->staged_buffer);
    assert(first_draw_index + draws <= singleton_data.max_draws);
    assert(first_multidraw_index + multidraw_count <= singleton_data.max_multidraw_draws);
    assert(mesh->index_count && mesh->vertex_count);

    PipelineOrProgram vpipeline;
    draw_setup_common(command_buffer, pipeline, &vpipeline, mesh, 0, -1, -1, 0, 0);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    
    const unsigned int align = VULKAN ? pigeon_vulkan_get_buffer_min_alignment()
        : pigeon_opengl_get_uniform_buffer_min_alignment();
    unsigned int o = round_up(sizeof(PigeonWGISceneUniformData), align);
    o += round_up(sizeof(PigeonWGIDrawObject) * singleton_data.max_draws, align);

    uint32_t pushc[2] = {first_draw_index, command_buffer->mvp_index};

    pigeon_vulkan_multidraw_indexed(&command_buffer->command_pool, 0, 
        vpipeline, sizeof pushc, &pushc,
        &objects->uniform_buffer, o,
        first_multidraw_index, multidraw_count);
}

PIGEON_ERR_RET pigeon_wgi_end_command_buffer(PigeonWGICommandBuffer * command_buffer)
{
    if(VULKAN) ASSERT_R1(!pigeon_vulkan_end_submission(&command_buffer->command_pool, 0));

    if(OPENGL) {
        command_buffer->has_been_recorded = true;
        PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

        PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();

        if(command_buffer == &objects->light_pass_command_buffer) {

            float dist_and_half[4] = {
                1.5f / (float)sc_info.width, 1.5f / (float)sc_info.height,
                0.5f / (float)sc_info.width, 0.5f / (float)sc_info.height,
            };

            pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_light_blur);
            pigeon_opengl_set_uniform_vec2(&singleton_data.gl.shader_light_blur, 
                singleton_data.gl.shader_light_blur_u_near_far,
                singleton_data.znear, singleton_data.zfar);
            pigeon_opengl_set_draw_state(&singleton_data.gl.full_screen_tri_cfg);


            for(unsigned int i = 0; i < singleton_data.render_cfg.shadow_blur_passes; i++) {
                pigeon_opengl_bind_framebuffer(&singleton_data.gl.light_blur1_framebuffer);
                pigeon_opengl_bind_texture(0, &singleton_data.light_image.gltex2d);
                pigeon_opengl_bind_texture(1, &singleton_data.depth_image.gltex2d);
                pigeon_opengl_set_uniform_vec4(&singleton_data.gl.shader_light_blur, 
                    singleton_data.gl.shader_light_blur_u_dist_and_half, 
                    dist_and_half[0], dist_and_half[1], dist_and_half[2], dist_and_half[3]);
                pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);

                dist_and_half[0] += 1.0f / (float)sc_info.width;
                dist_and_half[1] += 1.0f / (float)sc_info.height;

                pigeon_opengl_bind_framebuffer(&singleton_data.gl.light_blur2_framebuffer);
                pigeon_opengl_bind_texture(0, &singleton_data.light_blur_image.gltex2d);
                pigeon_opengl_set_uniform_vec4(&singleton_data.gl.shader_light_blur,
                    singleton_data.gl.shader_light_blur_u_dist_and_half, 
                    dist_and_half[0], dist_and_half[1], dist_and_half[2], dist_and_half[3]);
                pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);

                dist_and_half[0] += 1.0f / (float)sc_info.width;
                dist_and_half[1] += 1.0f / (float)sc_info.height;
            }
        }
        else if(command_buffer == &objects->render_command_buffer) {
            pigeon_opengl_set_draw_state(&singleton_data.gl.full_screen_tri_cfg);
            if(singleton_data.render_cfg.bloom) {
                pigeon_opengl_bind_framebuffer(&singleton_data.gl.bloom1_framebuffer);
                pigeon_opengl_bind_texture(0, &singleton_data.render_image.gltex2d);
                pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_bloom_downsample);

                pigeon_opengl_set_uniform_vec3(&singleton_data.gl.shader_bloom_downsample, 
                    singleton_data.gl.shader_bloom_downsample_u_offset_and_min,
                    1 / (float)sc_info.width, 1 / (float)sc_info.height,
                    5 // minimum light
                );

                pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);

                pigeon_opengl_bind_framebuffer(&singleton_data.gl.bloom2_framebuffer);
                pigeon_opengl_bind_texture(0, &singleton_data.bloom1_image.gltex2d);
                pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_blur);
                pigeon_opengl_set_uniform_vec2(&singleton_data.gl.shader_blur, 
                    singleton_data.gl.shader_blur_ONE_PIXEL,
                    0, 8 / (float)sc_info.height
                );
                pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);

                pigeon_opengl_bind_framebuffer(&singleton_data.gl.bloom1_framebuffer);
                pigeon_opengl_bind_texture(0, &singleton_data.bloom2_image.gltex2d);
                pigeon_opengl_set_uniform_vec2(&singleton_data.gl.shader_blur, 
                    singleton_data.gl.shader_blur_ONE_PIXEL,
                    8 / (float)sc_info.width, 0
                );
                pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);

                pigeon_opengl_bind_texture(1, &singleton_data.bloom1_image.gltex2d);
            }

            pigeon_opengl_bind_framebuffer(NULL);
            pigeon_opengl_bind_shader_program(&singleton_data.gl.shader_post);
            pigeon_opengl_bind_texture(0, &singleton_data.render_image.gltex2d);
            pigeon_opengl_bind_texture(1, &singleton_data.bloom1_image.gltex2d);


            pigeon_opengl_set_uniform_vec3(&singleton_data.gl.shader_post, 
                singleton_data.gl.shader_post_u_one_pixel_and_bloom_intensity,
                1 / (float)sc_info.width, 1 / (float)sc_info.height,
                singleton_data.bloom_intensity               
            );

            pigeon_opengl_draw(&singleton_data.gl.empty_vao, 0, 3);
            
        }
    }
    return 0;
}

static void use_secondary_command_buffer(PigeonVulkanCommandPool * primary, unsigned int primary_i,
    PigeonWGICommandBuffer * command_buffer, PigeonVulkanRenderPass * render_pass,
    PigeonVulkanFramebuffer * framebuffer, unsigned int viewport_width, unsigned int viewport_height)
{
    assert(command_buffer);

    if(render_pass) {
        assert(framebuffer && viewport_width && viewport_height);
        pigeon_vulkan_set_viewport_size(primary, primary_i, viewport_width, viewport_height);
        pigeon_vulkan_start_render_pass(primary, primary_i, render_pass, 
            framebuffer, viewport_width, viewport_height, true);
    }
    
    pigeon_vulkan_execute_secondary(primary, primary_i, &command_buffer->command_pool, 0);

    if(render_pass) {
        pigeon_vulkan_end_render_pass(primary, primary_i);
    }

}

static void do_light_blur(PerFrameData * objects, PigeonWGISwapchainInfo sc_info)
{    
    float blur_pushc[6] = {
        1.5f / (float)sc_info.width, 1.5f / (float)sc_info.height,
        0.5f / (float)sc_info.width, 0.5f / (float)sc_info.height,
        singleton_data.znear, singleton_data.zfar};

    unsigned int w = sc_info.width;
    unsigned int h = sc_info.height;

    PigeonVulkanCommandPool * p = &objects->primary_command_pool;

    for(unsigned int i = 0; i < singleton_data.render_cfg.shadow_blur_passes; i++) {
        pigeon_vulkan_set_viewport_size(&objects->primary_command_pool, 0, w, h);
        pigeon_vulkan_start_render_pass(p, 0, &singleton_data.rp_light_blur, 
            &singleton_data.light_blur1_framebuffer, w, h, false);
        pigeon_vulkan_bind_pipeline(p, 0, &singleton_data.pipeline_light_blur);
        pigeon_vulkan_bind_descriptor_set(p, 0, &singleton_data.pipeline_light_blur, &singleton_data.light_blur1_descriptor_pool, 0);
        pigeon_vulkan_draw(p, 0, 0, 3, 1, &singleton_data.pipeline_light_blur, sizeof blur_pushc, blur_pushc);
        pigeon_vulkan_end_render_pass(p, 0);
        pigeon_vulkan_wait_for_colour_write(p, 0, &singleton_data.light_blur_image.image);

        blur_pushc[0] += 1.0f / (float)sc_info.width;
        blur_pushc[1] += 1.0f / (float)sc_info.height;


        pigeon_vulkan_set_viewport_size(&objects->primary_command_pool, 0, w, h);
        pigeon_vulkan_start_render_pass(p, 0, &singleton_data.rp_light_blur, 
            &singleton_data.light_blur2_framebuffer, w, h, false);
        pigeon_vulkan_bind_pipeline(p, 0, &singleton_data.pipeline_light_blur);
        pigeon_vulkan_bind_descriptor_set(p, 0, &singleton_data.pipeline_light_blur, &singleton_data.light_blur2_descriptor_pool, 0);
        pigeon_vulkan_draw(p, 0, 0, 3, 1, &singleton_data.pipeline_light_blur, sizeof blur_pushc, blur_pushc);
        pigeon_vulkan_end_render_pass(p, 0);
        pigeon_vulkan_wait_for_colour_write(p, 0, &singleton_data.light_image.image);

        blur_pushc[0] += 1.0f / (float)sc_info.width;
        blur_pushc[1] += 1.0f / (float)sc_info.height;
    }

    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        pigeon_vulkan_set_timer(&objects->primary_command_pool, 0, &objects->timer_query_pool, PIGEON_WGI_TIMER_LIGHT_GAUSSIAN_BLUR_DONE);
    }
}

// TODO split this up
static PIGEON_ERR_RET do_post(PerFrameData * objects, PigeonWGISwapchainInfo sc_info)
{
    PigeonVulkanCommandPool * p = &objects->primary_command_pool;
    ASSERT_R1(!pigeon_vulkan_start_submission(p, 2));


    if(singleton_data.render_cfg.bloom) {
   
        // Generate bloom image from HDR (downsample)

        float downsample_pushc[3] = {
            1 / (float)sc_info.width, 1 / (float)sc_info.height,
            5 // minimum light
        };

        pigeon_vulkan_wait_for_colour_write(p, 2, &singleton_data.render_image.image);
        pigeon_vulkan_set_viewport_size(p, 2, sc_info.width/8, sc_info.height/8);
        pigeon_vulkan_start_render_pass(p, 2, &singleton_data.rp_bloom_blur, 
            &singleton_data.bloom1_framebuffer, sc_info.width/8, sc_info.height/8, false);
        pigeon_vulkan_bind_pipeline(p, 2, &singleton_data.pipeline_bloom_downsample);
        pigeon_vulkan_bind_descriptor_set(p, 2, &singleton_data.pipeline_bloom_downsample, &singleton_data.bloom_downsample_descriptor_pool, 0);
        pigeon_vulkan_draw(p, 2, 0, 3, 1, &singleton_data.pipeline_bloom_downsample, sizeof downsample_pushc, downsample_pushc);
        pigeon_vulkan_end_render_pass(p, 2);
        pigeon_vulkan_wait_for_colour_write(p, 2, &singleton_data.bloom1_image.image);

    }

    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        pigeon_vulkan_set_timer(&objects->primary_command_pool, 2, &objects->timer_query_pool, PIGEON_WGI_TIMER_BLOOM_DOWNSAMPLE_DONE);
    }
    
    if(singleton_data.render_cfg.bloom) {

        // Gaussian blur bloom image

        float blur_pushc[2] = {0, 8 / (float)sc_info.height};

        pigeon_vulkan_set_viewport_size(p, 2, sc_info.width/8, sc_info.height/8);
        pigeon_vulkan_start_render_pass(p, 2, &singleton_data.rp_bloom_blur, 
            &singleton_data.bloom2_framebuffer, sc_info.width/8, sc_info.height/8, false);
        pigeon_vulkan_bind_pipeline(p, 2, &singleton_data.pipeline_blur);
        pigeon_vulkan_bind_descriptor_set(p, 2, &singleton_data.pipeline_blur, &singleton_data.bloom1_descriptor_pool, 0);
        pigeon_vulkan_draw(p, 2, 0, 3, 1, &singleton_data.pipeline_blur, sizeof blur_pushc, blur_pushc);
        pigeon_vulkan_end_render_pass(p, 2);
        pigeon_vulkan_wait_for_colour_write(p, 2, &singleton_data.bloom2_image.image);


        float blur_pushc2[2] = {8 / (float)sc_info.width, 0};


        pigeon_vulkan_set_viewport_size(p, 2, sc_info.width/8, sc_info.height/8);
        pigeon_vulkan_start_render_pass(p, 2, &singleton_data.rp_bloom_blur, 
            &singleton_data.bloom1_framebuffer, sc_info.width/8, sc_info.height/8, false);
        pigeon_vulkan_bind_pipeline(p, 2, &singleton_data.pipeline_blur);
        pigeon_vulkan_bind_descriptor_set(p, 2, &singleton_data.pipeline_blur, &singleton_data.bloom2_descriptor_pool, 0);
        pigeon_vulkan_draw(p, 2, 0, 3, 1, &singleton_data.pipeline_blur, sizeof blur_pushc2, blur_pushc2);
        pigeon_vulkan_end_render_pass(p, 2);
        pigeon_vulkan_wait_for_colour_write(p, 2, &singleton_data.bloom1_image.image); 

    }


    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        pigeon_vulkan_set_timer(&objects->primary_command_pool, 2, &objects->timer_query_pool, PIGEON_WGI_TIMER_BLOOM_GAUSSIAN_BLUR_DONE);
    }


    // Post-process

    struct {
        float one_pixel_x, one_pixel_y;
        float bloom_intensity;
    } post_pushc;
    post_pushc.one_pixel_x = 1 / (float)sc_info.width;
    post_pushc.one_pixel_y = 1 / (float)sc_info.height;
    post_pushc.bloom_intensity = singleton_data.bloom_intensity;
    
    pigeon_vulkan_set_viewport_size(p, 2, sc_info.width, sc_info.height);
    pigeon_vulkan_start_render_pass(p, 2, &singleton_data.rp_post, 
        &singleton_data.post_framebuffers[singleton_data.swapchain_image_index], sc_info.width, sc_info.height, false);
    pigeon_vulkan_bind_pipeline(p, 2, &singleton_data.pipeline_post);
    pigeon_vulkan_bind_descriptor_set(p, 2, &singleton_data.pipeline_post, &singleton_data.post_process_descriptor_pool, 0);
    pigeon_vulkan_draw(p, 2, 0, 3, 1, &singleton_data.pipeline_post, sizeof post_pushc, &post_pushc);
    pigeon_vulkan_end_render_pass(p, 2);

    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        pigeon_vulkan_set_timer(&objects->primary_command_pool, 2, &objects->timer_query_pool, PIGEON_WGI_TIMER_POST_PROCESS_DONE);
    }

    return 0;
}

PIGEON_ERR_RET pigeon_wgi_present_frame()
{
    if(OPENGL) {
        pigeon_wgi_swap_buffers();

        singleton_data.previous_frame_index_mod = singleton_data.current_frame_index_mod;
        singleton_data.current_frame_index_mod = (singleton_data.current_frame_index_mod+1) % singleton_data.frame_objects_count;

        return 0;
    }

    const bool first_frame = singleton_data.previous_frame_index_mod == UINT32_MAX;

    PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();

    /* Preprocessing */


    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    PerFrameData * prev_objects = first_frame ? NULL : &singleton_data.per_frame_objects[singleton_data.previous_frame_index_mod];

    ASSERT_R1(objects->depth_command_buffer.has_been_recorded && objects->render_command_buffer.has_been_recorded);
    objects->first_frame_submitted = true;

    
    ASSERT_R1 (!pigeon_vulkan_start_submission(&objects->primary_command_pool, 0));

    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        pigeon_vulkan_reset_query_pool(&objects->primary_command_pool, 0, &objects->timer_query_pool);
        pigeon_vulkan_set_timer(&objects->primary_command_pool, 0, &objects->timer_query_pool, PIGEON_WGI_TIMER_START);
    }

    if(objects->upload_command_buffer.has_been_recorded) {
        use_secondary_command_buffer(&objects->primary_command_pool, 0,
            &objects->upload_command_buffer, NULL,  NULL, 0, 0);
        objects->upload_command_buffer.has_been_recorded = false;

        // secondary command buffer already includes layout transitions & barriers for images
        pigeon_vulkan_wait_for_vertex_data_transfer(&objects->primary_command_pool, 0);
    }

    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        pigeon_vulkan_set_timer(&objects->primary_command_pool, 0, &objects->timer_query_pool, PIGEON_WGI_TIMER_UPLOAD_DONE);
    }

    use_secondary_command_buffer(&objects->primary_command_pool, 0,
        &objects->depth_command_buffer, &singleton_data.rp_depth,
        &singleton_data.depth_framebuffer, sc_info.width, sc_info.height);

    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        pigeon_vulkan_set_timer(&objects->primary_command_pool, 0, &objects->timer_query_pool, PIGEON_WGI_TIMER_DEPTH_PREPASS_DONE);
    }

    // Shadows
    for(unsigned int i = 0; i < 4; i++) {
        PigeonWGIShadowParameters* p = &singleton_data.shadow_parameters[i];
        if(!p->resolution) continue;

        ASSERT_R1(p->framebuffer_index >= 0);
        ASSERT_R1(objects->shadow_command_buffers[i].has_been_recorded);

        unsigned int j = (unsigned)p->framebuffer_index;
        unsigned int w = singleton_data.shadow_images[j].image.width;
        unsigned int h = singleton_data.shadow_images[j].image.height;
        PigeonVulkanFramebuffer* fb = &singleton_data.shadow_framebuffers[j];
    
        use_secondary_command_buffer(&objects->primary_command_pool, 0,
            &objects->shadow_command_buffers[i], &singleton_data.rp_depth,
            fb, w, h);
    }

    pigeon_vulkan_wait_for_depth_write(&objects->primary_command_pool, 0, &singleton_data.depth_image.image);
    for(unsigned int i = 0; i < 4; i++) {
        PigeonWGIShadowParameters* p = &singleton_data.shadow_parameters[i];
        if(!p->resolution) continue;
        pigeon_vulkan_wait_for_depth_write(&objects->primary_command_pool, 0, &singleton_data.shadow_images[i].image);
    }

    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        pigeon_vulkan_set_timer(&objects->primary_command_pool, 0, &objects->timer_query_pool, PIGEON_WGI_TIMER_SHADOW_MAPS_DONE);
    }

    use_secondary_command_buffer(&objects->primary_command_pool, 0,
        &objects->light_pass_command_buffer, &singleton_data.rp_light_pass,
        &singleton_data.light_framebuffer, sc_info.width, sc_info.height);
    pigeon_vulkan_wait_for_colour_write(&objects->primary_command_pool, 0, &singleton_data.light_image.image);


    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        pigeon_vulkan_set_timer(&objects->primary_command_pool, 0, &objects->timer_query_pool, PIGEON_WGI_TIMER_LIGHT_PASS_DONE);
    }


    do_light_blur(objects, sc_info);


    ASSERT_R1(!pigeon_vulkan_submit3(&objects->primary_command_pool, 0, &objects->pre_render_done_fence, 
        first_frame ? NULL : &prev_objects->render_done_semaphore2, NULL,
        &objects->pre_processing_done_semaphore, NULL));



    /* Render */    

    ASSERT_R1 (!pigeon_vulkan_start_submission(&objects->primary_command_pool, 1));

    // Wait for light data


    use_secondary_command_buffer(&objects->primary_command_pool, 1,
        &objects->render_command_buffer, &singleton_data.rp_render,
        &singleton_data.render_framebuffer, sc_info.width, sc_info.height);
    pigeon_vulkan_wait_for_colour_write(&objects->primary_command_pool, 1, &singleton_data.render_image.image);
    

    if(pigeon_vulkan_general_queue_supports_timestamps()) {
        pigeon_vulkan_set_timer(&objects->primary_command_pool, 1, &objects->timer_query_pool, PIGEON_WGI_TIMER_RENDER_DONE);
    }


    ASSERT_R1(!pigeon_vulkan_submit3(&objects->primary_command_pool, 1, NULL, 
        first_frame ? NULL : &prev_objects->post_processing_done_semaphore2, &objects->pre_processing_done_semaphore,
        &objects->render_done_semaphore, &objects->render_done_semaphore2));


    /* Post-processing */

    ASSERT_R1(!do_post(objects, sc_info));
    ASSERT_R1(!pigeon_vulkan_submit3(&objects->primary_command_pool, 2, NULL, 
        &objects->render_done_semaphore, &objects->swapchain_acquire_semaphore,
        &objects->post_processing_done_semaphore, &objects->post_processing_done_semaphore2));




    singleton_data.previous_frame_index_mod = singleton_data.current_frame_index_mod;
    singleton_data.current_frame_index_mod = (singleton_data.current_frame_index_mod+1) % singleton_data.frame_objects_count;

    /* Swapchain */

    int swapchain_err = pigeon_vulkan_swapchain_present(&objects->post_processing_done_semaphore);
    if(swapchain_err == 2) {
        ASSERT_R1(!recreate_swapchain_loop());
        swapchain_err = 0;
    }

    return swapchain_err;
}


