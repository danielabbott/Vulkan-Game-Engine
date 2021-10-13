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


PIGEON_ERR_RET pigeon_wgi_create_per_frame_objects()
{
    singleton_data.swapchain_image_index = UINT32_MAX;
    singleton_data.previous_frame_index_mod = UINT32_MAX;

    PigeonVulkanSwapchainInfo sc_info = pigeon_vulkan_get_swapchain_info();
    singleton_data.frame_objects_count = sc_info.image_count;

    singleton_data.per_frame_objects = calloc(sc_info.image_count, sizeof *singleton_data.per_frame_objects);
    ASSERT_R1(singleton_data.per_frame_objects);

    singleton_data.post_framebuffers = calloc(sc_info.image_count, sizeof *singleton_data.post_framebuffers);
    ASSERT_R1(singleton_data.post_framebuffers);

    for(unsigned int i = 0; i < singleton_data.frame_objects_count; i++) {
        PerFrameData * objects = &singleton_data.per_frame_objects[i];
        ASSERT_R1(!pigeon_vulkan_create_command_pool(&objects->primary_command_pool, 3, true, false));

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
        for(unsigned int j = 0; j < 90; j++) {
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
        

        
        ASSERT_R1(!pigeon_vulkan_create_timer_query_pool(&objects->timer_query_pool, PIGEON_WGI_TIMERS_COUNT));


        // Framebuffer is specific to swapchain image - NOT current frame index mod

        ASSERT_R1(!pigeon_vulkan_create_framebuffer(&singleton_data.post_framebuffers[i],
            NULL, pigeon_vulkan_get_swapchain_image_view(i), &singleton_data.rp_post));

    }
    return 0;
}

void pigeon_wgi_destroy_per_frame_objects()
{
    if(!singleton_data.per_frame_objects) return;

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

    free(singleton_data.per_frame_objects);
    free(singleton_data.post_framebuffers);

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
    ASSERT_R1(!pigeon_vulkan_reset_command_pool(&singleton_data.per_frame_objects[singleton_data.current_frame_index_mod].primary_command_pool));
   
    return 0;
}


static PIGEON_ERR_RET prepare_uniform_buffers()
{
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    const unsigned int align = pigeon_vulkan_get_uniform_buffer_min_alignment();

    unsigned int minimum_size = round_up(sizeof(PigeonWGISceneUniformData), align);
    minimum_size += round_up(sizeof(PigeonWGIDrawObject) * singleton_data.max_draws, align);
    minimum_size += round_up(sizeof(PigeonVulkanDrawIndexedIndirectCommand) * singleton_data.max_draws, align);
    minimum_size += singleton_data.max_bones * 4*3*4;

    
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
        uniform_offset += round_up(sizeof(PigeonVulkanDrawIndexedIndirectCommand) * singleton_data.max_draws, align);
            
        pigeon_vulkan_set_descriptor_ssbo2(&objects->render_descriptor_pool, 0, 2, 0,
            &objects->uniform_buffer, uniform_offset,
            4*3*4 * singleton_data.max_bones);
        pigeon_vulkan_set_descriptor_ssbo2(&objects->depth_descriptor_pool, 0, 2, 0,
            &objects->uniform_buffer, uniform_offset,
            4*3*4 * singleton_data.max_bones);
        pigeon_vulkan_set_descriptor_ssbo2(&objects->light_pass_descriptor_pool, 0, 2, 0,
            &objects->uniform_buffer, uniform_offset,
            4*3*4 * singleton_data.max_bones);
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

PIGEON_ERR_RET pigeon_wgi_next_frame_poll(double delayed_timer_values[PIGEON_WGI_TIMERS_COUNT], bool* ready)
{
    pigeon_wgi_poll_events();

    bool block = ready == NULL;

    
    PerFrameData * prev_objects = NULL;
    if(singleton_data.previous_frame_index_mod != UINT32_MAX)
        prev_objects = &singleton_data.per_frame_objects[singleton_data.previous_frame_index_mod];

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

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


    int swapchain_err = pigeon_vulkan_next_swapchain_image(&singleton_data.swapchain_image_index, 
        &objects->swapchain_acquire_semaphore, NULL, block);

    if(swapchain_err == 3) {
        // not ready yet
        ASSERT_R1(!block);
        *ready = false;
        return 0;
    }
    else if(swapchain_err == 2) {
        // recreate swapchain
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
    }
    else {
        ASSERT_R1(!swapchain_err);
    }

    if(!objects->first_frame_submitted ||
        pigeon_vulkan_get_timer_results(&objects->timer_query_pool, delayed_timer_values))
    {        
        memset(delayed_timer_values, 0, sizeof(double) * PIGEON_WGI_TIMERS_COUNT);
    }

    if(ready) *ready = true;
    return 0;
    
}


PIGEON_ERR_RET pigeon_wgi_start_frame(unsigned int max_draws,
    uint32_t max_multidraw_draws,
    PigeonWGIShadowParameters shadows[4], unsigned int max_bones)
{
    ASSERT_R1(max_draws <= 65536);
    ASSERT_R1(max_bones <= max_draws*256);
    ASSERT_R1(max_multidraw_draws <= max_draws);

    if(max_draws > singleton_data.max_draws) singleton_data.max_draws = max_draws;
    if(!singleton_data.max_draws) singleton_data.max_draws = 128;
    
    if(max_multidraw_draws > singleton_data.max_multidraw_draws)
        singleton_data.max_multidraw_draws = max_multidraw_draws;
    if(!singleton_data.max_multidraw_draws) singleton_data.max_multidraw_draws = 128;
    
    if(max_bones > singleton_data.max_bones) singleton_data.max_bones = max_bones;
    if(!singleton_data.max_bones) singleton_data.max_bones = 128;

    singleton_data.multidraw_draw_index = 0;

    
    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod]; 


    memcpy(singleton_data.shadow_parameters, shadows, sizeof singleton_data.shadow_parameters);
    ASSERT_R1(!pigeon_wgi_assign_shadow_framebuffers());

    ASSERT_R1(!reset_buffers());
    objects->commands_in_progress = true;
    ASSERT_R1(!prepare_uniform_buffers());


    // Bind shadow textures
    for(unsigned int i = 0; i < 4; i++) {
        PigeonWGIShadowParameters* p = &singleton_data.shadow_parameters[i];
        if(!p->resolution) {
            pigeon_vulkan_set_descriptor_texture(&objects->light_pass_descriptor_pool, 0, 4, i, 
                &singleton_data.default_shadow_map_image_view, &singleton_data.shadow_sampler);
            pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 4, i, 
                &singleton_data.default_shadow_map_image_view, &singleton_data.shadow_sampler);
            continue;
        }
        unsigned int j = (unsigned)p->framebuffer_index;
        pigeon_vulkan_set_descriptor_texture(&objects->light_pass_descriptor_pool, 0, 4, i, 
            &singleton_data.shadow_images[j].image_view, &singleton_data.shadow_sampler);
        pigeon_vulkan_set_descriptor_texture(&objects->render_descriptor_pool, 0, 4, i, 
            &singleton_data.shadow_images[j].image_view, &singleton_data.shadow_sampler);
    }

    return 0;
}

PIGEON_ERR_RET pigeon_wgi_set_uniform_data(PigeonWGISceneUniformData * uniform_data,
    PigeonWGIDrawObject * draw_objects, unsigned int draws_count,
    PigeonWGIBoneMatrix* bones, unsigned int bones_count)
{
    if(bones_count) ASSERT_R1(bones);
    if(draws_count) ASSERT_R1(draw_objects)

    uniform_data->znear = singleton_data.znear;
    uniform_data->zfar = singleton_data.zfar;
    pigeon_wgi_set_shadow_uniforms(uniform_data, draw_objects, draws_count);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];

    assert(draws_count <= singleton_data.max_draws);

    uint8_t * dst = objects->uniform_buffer_memory.mapping;

    const unsigned int align = pigeon_vulkan_get_uniform_buffer_min_alignment();

    memcpy(dst, uniform_data, sizeof(PigeonWGISceneUniformData));
    dst += round_up(sizeof(PigeonWGISceneUniformData), align);

    memcpy(dst, draw_objects, draws_count * sizeof(PigeonWGIDrawObject));
    dst += round_up(sizeof(PigeonWGIDrawObject) * draws_count, align);
    dst += round_up(sizeof(PigeonVulkanDrawIndexedIndirectCommand) * draws_count, align);

    memcpy(dst, bones, bones_count * sizeof(PigeonWGIBoneMatrix));


    ASSERT_R1(!pigeon_vulkan_flush_memory(&objects->uniform_buffer_memory, 0, 0));

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


PIGEON_ERR_RET pigeon_wgi_start_command_buffer(PigeonWGICommandBuffer * command_buffer)
{
    ASSERT_R1(!pigeon_vulkan_reset_command_pool(&command_buffer->command_pool));

    if(command_buffer->no_render) {
        ASSERT_R1(!pigeon_vulkan_start_submission2( &command_buffer->command_pool, 0, NULL, NULL));
    }
    else {
        PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
        PigeonVulkanSwapchainInfo sc_info = pigeon_vulkan_get_swapchain_info();

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

static PigeonVulkanPipeline* get_pipeline(PigeonWGICommandBuffer* command_buffer, PigeonWGIPipeline* pipeline)
{
    return command_buffer->depth_only ?
        (command_buffer->shadow ? pipeline->pipeline_shadow_map : pipeline->pipeline_depth)
            : (command_buffer->light_pass ? pipeline->pipeline_light : pipeline->pipeline);
}

void pigeon_wgi_draw_without_mesh(PigeonWGICommandBuffer* command_buffer, PigeonWGIPipeline* pipeline, 
    unsigned int vertices)
{
    assert(command_buffer && pipeline && pipeline->pipeline && !command_buffer->depth_only);

    pigeon_vulkan_bind_pipeline(&command_buffer->command_pool, 0, pipeline->pipeline);
    pigeon_vulkan_bind_descriptor_set(&command_buffer->command_pool, 0, pipeline->pipeline, 
        &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod].render_descriptor_pool, 0);

    pigeon_vulkan_draw(&command_buffer->command_pool, 0, 0, vertices, 1,
        get_pipeline(command_buffer, pipeline), 0, NULL);
}

static void draw_setup_common(PigeonWGICommandBuffer* command_buffer, PigeonWGIPipeline* pipeline, 
    PigeonVulkanPipeline ** vpipeline, PigeonWGIMultiMesh* mesh)
{
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
    uint32_t draw_index, uint32_t instances, uint32_t first, uint32_t count)
{
    assert(command_buffer && pipeline && pipeline->pipeline && mesh && mesh->staged_buffer);
    if(!instances) instances = 1;
    if(!count) return;

    PigeonVulkanPipeline * vpipeline;
    draw_setup_common(command_buffer, pipeline, &vpipeline, mesh);

    uint32_t pushc[2] = {draw_index, command_buffer->mvp_index};
    
    if(mesh->index_count) {
        if(count == UINT32_MAX) {
            count = mesh->index_count - first;
        }

        pigeon_vulkan_draw_indexed(&command_buffer->command_pool, 0, 
            start_vertex, first, count, instances, vpipeline, sizeof pushc, &pushc);
    }
    else {
        if(count == UINT32_MAX) {
            count = mesh->vertex_count - first;
        }

        assert(!start_vertex);

        pigeon_vulkan_draw(&command_buffer->command_pool, 0, 
            first, count, instances, vpipeline, sizeof pushc, &pushc);
    }
}



void pigeon_wgi_multidraw_draw(unsigned int start_vertex,
	uint32_t instances, uint32_t first, uint32_t count, uint32_t first_instance)
{
    if(!instances || !count) {
        assert(false);
        return;
    }

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    const unsigned int align = pigeon_vulkan_get_uniform_buffer_min_alignment();

    unsigned int o = round_up(sizeof(PigeonWGISceneUniformData), align);
    o += round_up(sizeof(PigeonWGIDrawObject) * singleton_data.max_draws, align);

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

    assert(command_buffer && pipeline && pipeline->pipeline && mesh && mesh->staged_buffer);
    assert(first_draw_index + draws <= singleton_data.max_draws);
    assert(first_multidraw_index + multidraw_count <= singleton_data.max_multidraw_draws);
    assert(mesh->index_count && mesh->vertex_count);

    PigeonVulkanPipeline * vpipeline;
    draw_setup_common(command_buffer, pipeline, &vpipeline, mesh);

    PerFrameData * objects = &singleton_data.per_frame_objects[singleton_data.current_frame_index_mod];
    
    const unsigned int align = pigeon_vulkan_get_uniform_buffer_min_alignment();
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
    ASSERT_R1(!pigeon_vulkan_end_submission(&command_buffer->command_pool, 0));
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

static void do_light_blur(PerFrameData * objects, PigeonVulkanSwapchainInfo sc_info)
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
static PIGEON_ERR_RET do_post(PerFrameData * objects, PigeonVulkanSwapchainInfo sc_info,
    bool debug_disable_bloom)
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
        float vp_w, vp_h;
        float one_pixel_x, one_pixel_y;
        float bloom_intensity;
    } post_pushc;
    post_pushc.one_pixel_x = 1 / (float)sc_info.width;
    post_pushc.one_pixel_y = 1 / (float)sc_info.height;
    post_pushc.vp_w = (float)sc_info.width;
    post_pushc.vp_h = (float)sc_info.height;
    post_pushc.bloom_intensity = debug_disable_bloom ? 0 : 1;
    
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


PIGEON_ERR_RET pigeon_wgi_present_frame(bool debug_disable_bloom)
{
    const bool first_frame = singleton_data.previous_frame_index_mod == UINT32_MAX;

    PigeonVulkanSwapchainInfo sc_info = pigeon_vulkan_get_swapchain_info();

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

    ASSERT_R1(!do_post(objects, sc_info, debug_disable_bloom));
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


