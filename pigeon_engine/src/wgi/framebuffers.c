#include <pigeon/wgi/wgi.h>
#include <pigeon/util.h>
#include <pigeon/wgi/vulkan/image.h>
#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/wgi/vulkan/framebuffer.h>
#include <pigeon/wgi/window.h>
#include "singleton.h"




static int create_framebuffer_images(FramebufferImageObjects * objects,
    PigeonWGIImageFormat format, unsigned int width, unsigned int height,
    bool to_be_transfer_src, bool to_be_transfer_dst)
{
    assert(objects && format && width && height);

    PigeonVulkanMemoryRequirements memory_requirements;

    ASSERT_1(!pigeon_vulkan_create_image(&objects->image, format, width, height, 1, 1, false, false, true, true, 
        to_be_transfer_src, to_be_transfer_dst, &memory_requirements));
    

    static const PigeonVulkanMemoryTypePerferences memory_preferences = { 
        .device_local = PIGEON_VULKAN_MEMORY_TYPE_MUST,
        .host_visible = PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT,
        .host_coherent = PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT,
        .host_cached = PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT
    };

	if (pigeon_vulkan_allocate_memory_dedicated(&objects->memory, memory_requirements, memory_preferences,
        &objects->image, NULL)) return 1;
	if (pigeon_vulkan_image_bind_memory_dedicated(&objects->image, &objects->memory)) return 1;

    ASSERT_1(!pigeon_vulkan_create_image_view(&objects->image_view, &objects->image, false));

    return 0;
}

ERROR_RETURN_TYPE pigeon_wgi_create_framebuffers(void)
{
    PigeonVulkanSwapchainInfo sc_info = pigeon_vulkan_get_swapchain_info();
    
    if(create_framebuffer_images(&singleton_data.depth_image, PIGEON_WGI_IMAGE_FORMAT_DEPTH_U16, 
        sc_info.width, sc_info.height, false, false)) return 1;
    if(create_framebuffer_images(&singleton_data.ssao_image, PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR, 
        sc_info.width, sc_info.height, false, false)) return 1;
    if(create_framebuffer_images(&singleton_data.ssao_blur_image, PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR, 
        sc_info.width/2, sc_info.height, false, false)) return 1;
    if(create_framebuffer_images(&singleton_data.ssao_blur_image2, PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR, 
        sc_info.width/2, sc_info.height/2, false, false)) return 1;
    if(create_framebuffer_images(&singleton_data.render_image, PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR, 
        sc_info.width, sc_info.height, true, false)) return 1;
    if(create_framebuffer_images(&singleton_data.bloom_image, PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR, 
        sc_info.width/8, sc_info.height/8, false, true)) return 1;
    if(create_framebuffer_images(&singleton_data.bloom_gaussian_intermediate_image, PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR, 
        sc_info.width/8, sc_info.height/8, false, false)) return 1;


    create_framebuffer(&singleton_data.depth_framebuffer,
        &singleton_data.depth_image.image_view, NULL, &singleton_data.rp_depth);
    create_framebuffer(&singleton_data.ssao_framebuffer, 
        NULL, &singleton_data.ssao_image.image_view, &singleton_data.rp_ssao);
    create_framebuffer(&singleton_data.ssao_blur_framebuffer, 
        NULL, &singleton_data.ssao_blur_image.image_view, &singleton_data.rp_ssao_blur);
    create_framebuffer(&singleton_data.ssao_blur_framebuffer2, 
        NULL, &singleton_data.ssao_blur_image2.image_view, &singleton_data.rp_ssao_blur);
    create_framebuffer(&singleton_data.render_framebuffer, 
        &singleton_data.depth_image.image_view, &singleton_data.render_image.image_view, 
        &singleton_data.rp_render);
    create_framebuffer(&singleton_data.bloom_framebuffer, 
        NULL, &singleton_data.bloom_image.image_view, 
        &singleton_data.rp_bloom_gaussian);
    create_framebuffer(&singleton_data.bloom_gaussian_intermediate_framebuffer, 
        NULL, &singleton_data.bloom_gaussian_intermediate_image.image_view, 
        &singleton_data.rp_bloom_gaussian);

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
    if(singleton_data.ssao_framebuffer.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.ssao_framebuffer);
    if(singleton_data.ssao_blur_framebuffer.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.ssao_blur_framebuffer);
    if(singleton_data.ssao_blur_framebuffer2.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.ssao_blur_framebuffer2);
    if(singleton_data.render_framebuffer.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.render_framebuffer);
    if(singleton_data.bloom_gaussian_intermediate_framebuffer.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.bloom_gaussian_intermediate_framebuffer);
    if(singleton_data.bloom_framebuffer.vk_framebuffer) 
        pigeon_vulkan_destroy_framebuffer(&singleton_data.bloom_framebuffer);

    destroy_framebuffer_images(&singleton_data.depth_image);
    destroy_framebuffer_images(&singleton_data.ssao_image);
    destroy_framebuffer_images(&singleton_data.ssao_blur_image);
    destroy_framebuffer_images(&singleton_data.ssao_blur_image2);
    destroy_framebuffer_images(&singleton_data.render_image);
    destroy_framebuffer_images(&singleton_data.bloom_image);
    destroy_framebuffer_images(&singleton_data.bloom_gaussian_intermediate_image);
}


