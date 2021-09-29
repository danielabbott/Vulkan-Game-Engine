#include <pigeon/wgi/wgi.h>
#include <pigeon/util.h>
#include <pigeon/wgi/vulkan/image.h>
#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/wgi/vulkan/framebuffer.h>
#include <pigeon/wgi/vulkan/vulkan.h>
#include <pigeon/wgi/window.h>
#include "singleton.h"




int pigeon_wgi_create_framebuffer_images(FramebufferImageObjects * objects,
    PigeonWGIImageFormat format, unsigned int width, unsigned int height,
    bool to_be_transfer_src, bool to_be_transfer_dst,
    bool used_as_attatchment, bool used_as_storage_image)
{
    assert(objects && format && width && height);

    PigeonVulkanMemoryRequirements memory_requirements;

    ASSERT_1(!pigeon_vulkan_create_image(&objects->image, format, width, height, 1, 1,
        false, false, true, used_as_attatchment, used_as_storage_image, 
        to_be_transfer_src, to_be_transfer_dst, &memory_requirements));
    
    static const PigeonVulkanMemoryTypePreferences memory_preferences = { 
        .device_local = PIGEON_VULKAN_MEMORY_TYPE_MUST,
        .host_visible = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT,
        .host_coherent = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT,
        .host_cached = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT
    };

	ASSERT_1(!pigeon_vulkan_allocate_memory_dedicated(&objects->memory, memory_requirements, memory_preferences,
        &objects->image, NULL));
	ASSERT_1(!pigeon_vulkan_image_bind_memory_dedicated(&objects->image, &objects->memory));


    ASSERT_1(!pigeon_vulkan_create_image_view(&objects->image_view, &objects->image, false));

    return 0;
}

ERROR_RETURN_TYPE pigeon_wgi_create_framebuffers(void)
{
    PigeonVulkanSwapchainInfo sc_info = pigeon_vulkan_get_swapchain_info();

    PigeonWGIImageFormat hdr_format = pigeon_vulkan_compact_hdr_framebuffer_available() ?
        PIGEON_WGI_IMAGE_FORMAT_B10G11R11_UF_LINEAR : PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR;


    /* depth */
    
    if(pigeon_wgi_create_framebuffer_images(&singleton_data.depth_image, PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32, 
        sc_info.width, sc_info.height, false, false, true, false)) return 1;

    ASSERT_1(!pigeon_vulkan_create_framebuffer(&singleton_data.depth_framebuffer,
        &singleton_data.depth_image.image_view, NULL, &singleton_data.rp_depth));

   
    /* light */

    if(pigeon_wgi_create_framebuffer_images(&singleton_data.light_image, singleton_data.light_framebuffer_image_format, 
        sc_info.width, sc_info.height, false, false, true, true)) return 1;

    ASSERT_1(!pigeon_vulkan_create_framebuffer(&singleton_data.light_framebuffer, 
        &singleton_data.depth_image.image_view, &singleton_data.light_image.image_view,
        &singleton_data.rp_light_pass));


    /* light blur */
    
    if(pigeon_wgi_create_framebuffer_images(&singleton_data.light_blur_image, singleton_data.light_framebuffer_image_format, 
        sc_info.width, sc_info.height, false, false, false, true)) return 1;

        


    /* render */

    if(pigeon_wgi_create_framebuffer_images(&singleton_data.render_image, hdr_format, 
        sc_info.width, sc_info.height, false, false, true, false)) return 1;

    ASSERT_1(!pigeon_vulkan_create_framebuffer(&singleton_data.render_framebuffer, 
        &singleton_data.depth_image.image_view, &singleton_data.render_image.image_view, 
        &singleton_data.rp_render));

    
    /* bloom */

    if(singleton_data.render_graph.bloom) {
        if(pigeon_wgi_create_framebuffer_images(&singleton_data.bloom1_image, hdr_format, 
            sc_info.width/8, sc_info.height/8, false, false, true, false)) return 1;
        if(pigeon_wgi_create_framebuffer_images(&singleton_data.bloom2_image, hdr_format, 
            sc_info.width/8, sc_info.height/8, false, false, true, false)) return 1;

        ASSERT_1(!pigeon_vulkan_create_framebuffer(&singleton_data.bloom1_framebuffer, 
            NULL, &singleton_data.bloom1_image.image_view, &singleton_data.rp_bloom_blur));
        ASSERT_1(!pigeon_vulkan_create_framebuffer(&singleton_data.bloom2_framebuffer, 
            NULL, &singleton_data.bloom2_image.image_view, &singleton_data.rp_bloom_blur));
    }



    return 0;
}

static void destroy_framebuffer_images(FramebufferImageObjects * objects)
{
    if (objects->image_view.vk_image_view) pigeon_vulkan_destroy_image_view(&objects->image_view);
    if (objects->image.vk_image) pigeon_vulkan_destroy_image(&objects->image);
	if (objects->memory.vk_device_memory) pigeon_vulkan_free_memory(&objects->memory);
}

void pigeon_wgi_destroy_framebuffers(void)
{
    if(singleton_data.depth_framebuffer.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.depth_framebuffer);
    if(singleton_data.light_framebuffer.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.light_framebuffer);
    if(singleton_data.render_framebuffer.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.render_framebuffer);
    if(singleton_data.bloom1_framebuffer.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.bloom1_framebuffer);
    if(singleton_data.bloom2_framebuffer.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.bloom2_framebuffer);

    destroy_framebuffer_images(&singleton_data.depth_image);
    destroy_framebuffer_images(&singleton_data.light_image);
    destroy_framebuffer_images(&singleton_data.light_blur_image);
    destroy_framebuffer_images(&singleton_data.render_image);
    destroy_framebuffer_images(&singleton_data.bloom1_image);
    destroy_framebuffer_images(&singleton_data.bloom2_image);
}


