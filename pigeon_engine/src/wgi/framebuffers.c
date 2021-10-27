#include <pigeon/wgi/wgi.h>
#include <pigeon/assert.h>
#include <pigeon/wgi/vulkan/image.h>
#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/wgi/vulkan/framebuffer.h>
#include <pigeon/wgi/vulkan/vulkan.h>
#include <pigeon/wgi/opengl/framebuffer.h>
#include <pigeon/wgi/window.h>
#include "singleton.h"




PIGEON_ERR_RET pigeon_wgi_create_framebuffer_images(FramebufferImageObjects * objects,
    PigeonWGIImageFormat format, unsigned int width, unsigned int height,
    bool to_be_transfer_src, bool to_be_transfer_dst, bool shadow)
{
    assert(objects && format && width && height);

    if(VULKAN) {
        PigeonVulkanMemoryRequirements memory_requirements;

        ASSERT_R1(!pigeon_vulkan_create_image(&objects->image, format, width, height, 1, 1, false, false, true, true, 
            to_be_transfer_src, to_be_transfer_dst, &memory_requirements));
        
        static const PigeonVulkanMemoryTypePreferences memory_preferences = { 
            .device_local = PIGEON_VULKAN_MEMORY_TYPE_MUST,
            .host_visible = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT,
            .host_coherent = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT,
            .host_cached = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT
        };

        if (pigeon_vulkan_allocate_memory_dedicated(&objects->memory, memory_requirements, memory_preferences,
            &objects->image, NULL)) return 1;
        if (pigeon_vulkan_image_bind_memory_dedicated(&objects->image, &objects->memory)) return 1;

        ASSERT_R1(!pigeon_vulkan_create_image_view(&objects->image_view, &objects->image, false));
    }
    else {
        ASSERT_R1(!pigeon_opengl_create_texture(&objects->gltex2d, format, width, height, 0, 1));

        pigeon_opengl_set_texture_sampler(&objects->gltex2d,
            !pigeon_wgi_image_format_is_depth(format), false, shadow, true, false);
    }

    return 0;
}


PIGEON_ERR_RET pigeon_wgi_create_framebuffers(void)
{
    PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();

    PigeonWGIImageFormat hdr_format = (OPENGL || pigeon_vulkan_compact_hdr_framebuffer_available()) ?
        PIGEON_WGI_IMAGE_FORMAT_B10G11R11_UF_LINEAR : PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR;


    /* depth */
    
    if(pigeon_wgi_create_framebuffer_images(&singleton_data.depth_image, PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32, 
        sc_info.width, sc_info.height, false, false, false)) return 1;

    if(VULKAN) {
        ASSERT_R1(!pigeon_vulkan_create_framebuffer(&singleton_data.depth_framebuffer,
            &singleton_data.depth_image.image_view, NULL, &singleton_data.rp_depth));
    }
    else {
        ASSERT_R1(!pigeon_opengl_create_framebuffer(&singleton_data.gl.depth_framebuffer,
            &singleton_data.depth_image.gltex2d, NULL));
    }

   
    /* light */

    if(pigeon_wgi_create_framebuffer_images(&singleton_data.light_image, singleton_data.light_framebuffer_image_format, 
        sc_info.width, sc_info.height, false, false, false)) return 1;

    if(VULKAN) {
        ASSERT_R1(!pigeon_vulkan_create_framebuffer(&singleton_data.light_framebuffer, 
            &singleton_data.depth_image.image_view, &singleton_data.light_image.image_view,
            &singleton_data.rp_light_pass));
    }
    else {
        ASSERT_R1(!pigeon_opengl_create_framebuffer(&singleton_data.gl.light_framebuffer,
            &singleton_data.depth_image.gltex2d, &singleton_data.light_image.gltex2d));
    }


    /* light blur */
    
    if(pigeon_wgi_create_framebuffer_images(&singleton_data.light_blur_image, singleton_data.light_framebuffer_image_format, 
        sc_info.width, sc_info.height, false, false, false)) return 1;

    if(VULKAN) {
        ASSERT_R1(!pigeon_vulkan_create_framebuffer(&singleton_data.light_blur1_framebuffer,
            NULL, &singleton_data.light_blur_image.image_view, &singleton_data.rp_light_blur));
        ASSERT_R1(!pigeon_vulkan_create_framebuffer(&singleton_data.light_blur2_framebuffer,
            NULL, &singleton_data.light_image.image_view, &singleton_data.rp_light_blur));
    }
    else {
        ASSERT_R1(!pigeon_opengl_create_framebuffer(&singleton_data.gl.light_blur1_framebuffer,
            NULL, &singleton_data.light_blur_image.gltex2d));
        ASSERT_R1(!pigeon_opengl_create_framebuffer(&singleton_data.gl.light_blur2_framebuffer,
            NULL, &singleton_data.light_image.gltex2d));
    }

        


    /* render */

    if(pigeon_wgi_create_framebuffer_images(&singleton_data.render_image, hdr_format, 
        sc_info.width, sc_info.height, false, false, false)) return 1;

    if(VULKAN) {
        ASSERT_R1(!pigeon_vulkan_create_framebuffer(&singleton_data.render_framebuffer, 
            &singleton_data.depth_image.image_view, &singleton_data.render_image.image_view, 
            &singleton_data.rp_render));
    }
    else {
        ASSERT_R1(!pigeon_opengl_create_framebuffer(&singleton_data.gl.render_framebuffer, 
            &singleton_data.depth_image.gltex2d, &singleton_data.render_image.gltex2d));
    }

    
    /* bloom */

    if(singleton_data.render_cfg.bloom) {
        if(pigeon_wgi_create_framebuffer_images(&singleton_data.bloom_images[0][0], hdr_format, 
            sc_info.width/2, sc_info.height/2, false, false, false)) return 1;
        if(pigeon_wgi_create_framebuffer_images(&singleton_data.bloom_images[0][1], hdr_format, 
            sc_info.width/2, sc_info.height/2, false, false, false)) return 1;
        if(pigeon_wgi_create_framebuffer_images(&singleton_data.bloom_images[1][0], hdr_format, 
            sc_info.width/4, sc_info.height/4, false, false, false)) return 1;
        if(pigeon_wgi_create_framebuffer_images(&singleton_data.bloom_images[1][1], hdr_format, 
            sc_info.width/4, sc_info.height/4, false, false, false)) return 1;
        if(pigeon_wgi_create_framebuffer_images(&singleton_data.bloom_images[2][0], hdr_format, 
            sc_info.width/8, sc_info.height/8, false, false, false)) return 1;
        if(pigeon_wgi_create_framebuffer_images(&singleton_data.bloom_images[2][1], hdr_format, 
            sc_info.width/8, sc_info.height/8, false, false, false)) return 1;

        if(VULKAN) {
            for(unsigned int i = 0; i < 3; i++) {
                for(unsigned int j = 0; j < 2; j++) {
                    ASSERT_R1(!pigeon_vulkan_create_framebuffer(&singleton_data.bloom_framebuffers[i][j], 
                        NULL, &singleton_data.bloom_images[i][j].image_view, &singleton_data.rp_bloom_blur));
                }
            }
        }
        else {
            for(unsigned int i = 0; i < 3; i++) {
                for(unsigned int j = 0; j < 2; j++) {
                    ASSERT_R1(!pigeon_opengl_create_framebuffer(&singleton_data.gl.bloom_framebuffers[i][j], 
                        NULL, &singleton_data.bloom_images[i][j].gltex2d));
                }
            }
        }
    }



    return 0;
}

static void destroy_framebuffer_images(FramebufferImageObjects * objects)
{
    if(VULKAN) {
        if (objects->image_view.vk_image_view) pigeon_vulkan_destroy_image_view(&objects->image_view);
        if (objects->image.vk_image) pigeon_vulkan_destroy_image(&objects->image);
        if (objects->memory.vk_device_memory) pigeon_vulkan_free_memory(&objects->memory);
    }
    else if(objects->gltex2d.id) pigeon_opengl_destroy_texture(&objects->gltex2d);
}

void pigeon_wgi_destroy_framebuffers(void)
{
    if(VULKAN) {
        pigeon_vulkan_destroy_framebuffer(&singleton_data.depth_framebuffer);
        pigeon_vulkan_destroy_framebuffer(&singleton_data.light_framebuffer);
        pigeon_vulkan_destroy_framebuffer(&singleton_data.light_blur1_framebuffer);
        pigeon_vulkan_destroy_framebuffer(&singleton_data.light_blur2_framebuffer);
        pigeon_vulkan_destroy_framebuffer(&singleton_data.render_framebuffer);
        for(unsigned int i = 0; i < 3; i++) {
            for(unsigned int j = 0; j < 2; j++) {
                pigeon_vulkan_destroy_framebuffer(&singleton_data.bloom_framebuffers[i][j]);
            }
        }
    }
    else {
        pigeon_opengl_destroy_framebuffer(&singleton_data.gl.depth_framebuffer);
        pigeon_opengl_destroy_framebuffer(&singleton_data.gl.light_framebuffer);
        pigeon_opengl_destroy_framebuffer(&singleton_data.gl.light_blur1_framebuffer);
        pigeon_opengl_destroy_framebuffer(&singleton_data.gl.light_blur2_framebuffer);
        pigeon_opengl_destroy_framebuffer(&singleton_data.gl.render_framebuffer);
        for(unsigned int i = 0; i < 3; i++) {
            for(unsigned int j = 0; j < 2; j++) {
                pigeon_opengl_destroy_framebuffer(&singleton_data.gl.bloom_framebuffers[i][j]);
            }
        }
    }

    destroy_framebuffer_images(&singleton_data.depth_image);
    destroy_framebuffer_images(&singleton_data.light_image);
    destroy_framebuffer_images(&singleton_data.light_blur_image);
    destroy_framebuffer_images(&singleton_data.render_image);
    for(unsigned int i = 0; i < 3; i++) {
        for(unsigned int j = 0; j < 2; j++) {
            destroy_framebuffer_images(&singleton_data.bloom_images[i][j]);
        }
    }\
}


