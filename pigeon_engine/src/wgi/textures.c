#include "singleton.h"

#include <pigeon/wgi/vulkan/sampler.h>
#include <pigeon/wgi/vulkan/descriptor.h>
#include <pigeon/wgi/vulkan/memory.h>
#include <pigeon/wgi/vulkan/image.h>
#include <pigeon/wgi/vulkan/fence.h>
#include <pigeon/wgi/vulkan/command.h>
#include <pigeon/wgi/textures.h>
#include <pigeon/util.h>
#include <stdlib.h>


ERROR_RETURN_TYPE pigeon_wgi_create_descriptor_layouts(void)
{
	PigeonVulkanDescriptorBinding bindings[5] = {{0}};

	/* HDR render */

	// Per-scene UBO data
	bindings[0].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_UNIFORM;
	bindings[0].vertex_shader_accessible = true;
	bindings[0].fragment_shader_accessible = true;
	bindings[0].elements = 1;

	// Per-draw-call UBO data
	bindings[1].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_SSBO;
	bindings[1].vertex_shader_accessible = true;
	bindings[1].fragment_shader_accessible = true;
	bindings[1].elements = 1;

	// SSAO texture
	bindings[2].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[2].vertex_shader_accessible = false;
	bindings[2].fragment_shader_accessible = true;
	bindings[2].elements = 1;

	// Shadow map textures
	bindings[3].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[3].vertex_shader_accessible = false;
	bindings[3].fragment_shader_accessible = true;
	bindings[3].elements = 4;

	// Textures
	bindings[4].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[4].vertex_shader_accessible = false;
	bindings[4].fragment_shader_accessible = true;
	bindings[4].elements = 90;

	ASSERT_1(!pigeon_vulkan_create_descriptor_layout(&singleton_data.render_descriptor_layout, 5, bindings));

	/* Depth */

	// No fragment shader for depth-only render
	bindings[0].fragment_shader_accessible = false;
	bindings[1].fragment_shader_accessible = false;
	bindings[2].fragment_shader_accessible = false;
	ASSERT_1(!pigeon_vulkan_create_descriptor_layout(&singleton_data.depth_descriptor_layout, 3, bindings));

	/* Shadow maps */

	if (pigeon_vulkan_create_descriptor_layout(&singleton_data.shadow_map_descriptor_layout, 3, bindings)) return 1;

	/* SAO */

	// Depth image or SSAO
	bindings[0].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[0].vertex_shader_accessible = false;
	bindings[0].fragment_shader_accessible = true;

	if (pigeon_vulkan_create_descriptor_layout(&singleton_data.one_texture_descriptor_layout, 1, bindings)) return 1;
	
	
	/* Post-processing */

	// HDR image
	bindings[0].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[0].vertex_shader_accessible = false;
	bindings[0].fragment_shader_accessible = true;
	
	// Bloom 1/16 image
	bindings[1].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[1].vertex_shader_accessible = false;
	bindings[1].fragment_shader_accessible = true;

	if (pigeon_vulkan_create_descriptor_layout(&singleton_data.post_descriptor_layout, 2, bindings)) return 1;

	return 0;
}


void pigeon_wgi_destroy_descriptor_layouts(void)
{
	if (singleton_data.render_descriptor_layout.handle)
		pigeon_vulkan_destroy_descriptor_layout(&singleton_data.render_descriptor_layout);
	if (singleton_data.depth_descriptor_layout.handle)
		pigeon_vulkan_destroy_descriptor_layout(&singleton_data.depth_descriptor_layout);
	if (singleton_data.shadow_map_descriptor_layout.handle)
		pigeon_vulkan_destroy_descriptor_layout(&singleton_data.shadow_map_descriptor_layout);
	if (singleton_data.one_texture_descriptor_layout.handle)
		pigeon_vulkan_destroy_descriptor_layout(&singleton_data.one_texture_descriptor_layout);
	if (singleton_data.post_descriptor_layout.handle)
		pigeon_vulkan_destroy_descriptor_layout(&singleton_data.post_descriptor_layout);
}

ERROR_RETURN_TYPE pigeon_wgi_create_samplers(void)
{
	ASSERT_1(!pigeon_vulkan_create_sampler(&singleton_data.nearest_filter_sampler, false, false, false, true, false));
	ASSERT_1(!pigeon_vulkan_create_sampler(&singleton_data.bilinear_sampler, true, false, false, true, false));
	ASSERT_1(!pigeon_vulkan_create_sampler(&singleton_data.shadow_sampler, false, false, true, true, false));
	ASSERT_1(!pigeon_vulkan_create_sampler(&singleton_data.texture_sampler, true, true, true, false, true));	
	return 0;
}

void pigeon_wgi_destroy_samplers(void)
{
	if (singleton_data.nearest_filter_sampler.vk_sampler) pigeon_vulkan_destroy_sampler(&singleton_data.nearest_filter_sampler);
	if (singleton_data.bilinear_sampler.vk_sampler) pigeon_vulkan_destroy_sampler(&singleton_data.bilinear_sampler);
	if (singleton_data.shadow_sampler.vk_sampler) pigeon_vulkan_destroy_sampler(&singleton_data.shadow_sampler);
	if (singleton_data.texture_sampler.vk_sampler) pigeon_vulkan_destroy_sampler(&singleton_data.texture_sampler);
}

// TODO use device-local memory and clear with vkCmdClearColorImage?
static ERROR_RETURN_TYPE create_default_image_objects(void)
{
	PigeonVulkanMemoryRequirements memory_req;

	if (pigeon_vulkan_create_image(
		&singleton_data.default_1px_white_texture_image,
		PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR,
		1, 1, 1, 1,
		true, true,
		true, false,
		false, false,
		&memory_req
	)) return 1;

	PigeonVulkanMemoryTypePerferences preferences = { 0 };
	preferences.device_local = PIGEON_VULKAN_MEMORY_TYPE_PREFERED;
	preferences.host_visible = PIGEON_VULKAN_MEMORY_TYPE_MUST;
	preferences.host_coherent = PIGEON_VULKAN_MEMORY_TYPE_PREFERED;
	preferences.host_cached = PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT;

	if (pigeon_vulkan_allocate_memory(&singleton_data.default_textures_memory, memory_req, preferences)) return 1;

	if (pigeon_vulkan_image_bind_memory(&singleton_data.default_1px_white_texture_image,
		&singleton_data.default_textures_memory, 0)) return 1;

	if (pigeon_vulkan_create_image_view(&singleton_data.default_1px_white_texture_image_view,
		&singleton_data.default_1px_white_texture_image, false)) return 1;
	if (pigeon_vulkan_create_image_view(&singleton_data.default_1px_white_texture_array_image_view,
		&singleton_data.default_1px_white_texture_image, true)) return 1;
	return 0;
}

static ERROR_RETURN_TYPE set_default_image_data(void)
{
	uint32_t* image_data = NULL;

	if (pigeon_vulkan_map_memory(&singleton_data.default_textures_memory, (void**)&image_data)) return 1;

	*image_data = UINT32_MAX;


	int err = pigeon_vulkan_flush_memory(&singleton_data.default_textures_memory, 0, 4);
	pigeon_vulkan_unmap_memory(&singleton_data.default_textures_memory);
	return err;
}

static ERROR_RETURN_TYPE transition_default_image()
{
	PigeonVulkanCommandPool command_pool;
	if (pigeon_vulkan_create_command_pool(&command_pool, 1, true, false)) return 1;

	PigeonVulkanFence fence;

	if (pigeon_vulkan_create_fence(&fence, false)) {
		pigeon_vulkan_destroy_command_pool(&command_pool);
		return 1;
	}

#define CLEANUP()	pigeon_vulkan_destroy_fence(&fence); \
					pigeon_vulkan_destroy_command_pool(&command_pool);

	if (pigeon_vulkan_start_submission(&command_pool, 0)) {
		CLEANUP();
		return 1;
	}

	pigeon_vulkan_transition_image_preinit_to_general(&command_pool, 0, &singleton_data.default_1px_white_texture_image);


	if (pigeon_vulkan_submit(&command_pool, 0, &fence)) {
		CLEANUP();
		return 1;
	}

	int err = pigeon_vulkan_wait_fence(&fence);
	
	CLEANUP();

	if(err) return 1;

#undef CLEANUP

	return 0;
}

ERROR_RETURN_TYPE pigeon_wgi_create_default_textures(void)
{
	if (create_default_image_objects()) return 1;
	if (set_default_image_data()) return 1;
	if (transition_default_image()) return 1;
	return 0;
}

void pigeon_wgi_destroy_default_textures(void)
{
	if (singleton_data.default_1px_white_texture_image_view.vk_image_view)
		pigeon_vulkan_destroy_image_view(&singleton_data.default_1px_white_texture_image_view);
	if (singleton_data.default_1px_white_texture_array_image_view.vk_image_view)
		pigeon_vulkan_destroy_image_view(&singleton_data.default_1px_white_texture_array_image_view);
	if (singleton_data.default_1px_white_texture_image.vk_image)
		pigeon_vulkan_destroy_image(&singleton_data.default_1px_white_texture_image);
	if (singleton_data.default_textures_memory.vk_device_memory)
		pigeon_vulkan_free_memory(&singleton_data.default_textures_memory);
}

ERROR_RETURN_TYPE pigeon_wgi_create_descriptor_pools(void)
{
	// TODO have 1 descriptor pool with multiple sets
	if(pigeon_vulkan_create_descriptor_pool(&singleton_data.ssao_descriptor_pool,
		1, &singleton_data.one_texture_descriptor_layout)) return 1;
	if(pigeon_vulkan_create_descriptor_pool(&singleton_data.ssao_blur_descriptor_pool,
		1, &singleton_data.one_texture_descriptor_layout)) return 1;
	if(pigeon_vulkan_create_descriptor_pool(&singleton_data.bloom_downsample_descriptor_pool,
		1, &singleton_data.one_texture_descriptor_layout)) return 1;
	if(pigeon_vulkan_create_descriptor_pool(&singleton_data.bloom_gaussian_descriptor_pool,
		1, &singleton_data.one_texture_descriptor_layout)) return 1;
	if(pigeon_vulkan_create_descriptor_pool(&singleton_data.bloom_intermediate_gaussian_descriptor_pool,
		1, &singleton_data.one_texture_descriptor_layout)) return 1;		
	if(pigeon_vulkan_create_descriptor_pool(&singleton_data.post_process_descriptor_pool,
		1, &singleton_data.post_descriptor_layout)) return 1;

	return 0;
}

void pigeon_wgi_set_global_descriptors(void)
{

	pigeon_vulkan_set_descriptor_texture(&singleton_data.ssao_descriptor_pool, 0, 0, 0, 
		&singleton_data.depth_image.image_view, &singleton_data.nearest_filter_sampler);

	pigeon_vulkan_set_descriptor_texture(&singleton_data.ssao_blur_descriptor_pool, 0, 0, 0, 
		&singleton_data.ssao_image.image_view, &singleton_data.bilinear_sampler);

	pigeon_vulkan_set_descriptor_texture(&singleton_data.bloom_downsample_descriptor_pool, 0, 0, 0, 
		&singleton_data.render_image.image_view, &singleton_data.bilinear_sampler);

	pigeon_vulkan_set_descriptor_texture(&singleton_data.bloom_gaussian_descriptor_pool, 0, 0, 0, 
		&singleton_data.bloom_image.image_view, &singleton_data.bilinear_sampler);

	pigeon_vulkan_set_descriptor_texture(&singleton_data.bloom_intermediate_gaussian_descriptor_pool, 0, 0, 0, 
		&singleton_data.bloom_gaussian_intermediate_image.image_view, &singleton_data.bilinear_sampler);	
		
	pigeon_vulkan_set_descriptor_texture(&singleton_data.post_process_descriptor_pool, 0, 0, 0, 
		&singleton_data.render_image.image_view, &singleton_data.nearest_filter_sampler);
	pigeon_vulkan_set_descriptor_texture(&singleton_data.post_process_descriptor_pool, 0, 1, 0, 
		&singleton_data.bloom_image.image_view, &singleton_data.bilinear_sampler);
}

void pigeon_wgi_destroy_descriptor_pools(void)
{
	if (singleton_data.ssao_descriptor_pool.vk_descriptor_pool) 
		pigeon_vulkan_destroy_descriptor_pool(&singleton_data.ssao_descriptor_pool);
	if (singleton_data.ssao_blur_descriptor_pool.vk_descriptor_pool) 
		pigeon_vulkan_destroy_descriptor_pool(&singleton_data.ssao_blur_descriptor_pool);
	if (singleton_data.bloom_downsample_descriptor_pool.vk_descriptor_pool) 
		pigeon_vulkan_destroy_descriptor_pool(&singleton_data.bloom_downsample_descriptor_pool);		
	if (singleton_data.bloom_gaussian_descriptor_pool.vk_descriptor_pool) 
		pigeon_vulkan_destroy_descriptor_pool(&singleton_data.bloom_gaussian_descriptor_pool);
	if (singleton_data.bloom_intermediate_gaussian_descriptor_pool.vk_descriptor_pool) 
		pigeon_vulkan_destroy_descriptor_pool(&singleton_data.bloom_intermediate_gaussian_descriptor_pool);
	if (singleton_data.post_process_descriptor_pool.vk_descriptor_pool) 
		pigeon_vulkan_destroy_descriptor_pool(&singleton_data.post_process_descriptor_pool);
	
}


unsigned int pigeon_image_format_bytes_per_pixel(PigeonWGIImageFormat f)
{
	switch (f) {
		case PIGEON_WGI_IMAGE_FORMAT_NONE:
			return 0;
		case PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR:
			return 1;
		case PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR:
			return 2;
		case PIGEON_WGI_IMAGE_FORMAT_DEPTH_U24:
			return 3;
		case PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR:
		case PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB:
		case PIGEON_WGI_IMAGE_FORMAT_BGRA_U8_SRGB:
		case PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32:
			return 4;
		case PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR:
			return 8;
		default:
			;
	}
	assert(false);
	return 0;
}

unsigned int pigeon_image_format_bytes_per_4x4_block(PigeonWGIImageFormat f)
{
	switch (f) {		
		case PIGEON_WGI_IMAGE_FORMAT_BC1_SRGB:
		case PIGEON_WGI_IMAGE_FORMAT_ETC1_LINEAR:
		case PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB:
			return 8;
		case PIGEON_WGI_IMAGE_FORMAT_BC3_SRGB:
		case PIGEON_WGI_IMAGE_FORMAT_BC5_SRGB:
		case PIGEON_WGI_IMAGE_FORMAT_BC7_SRGB:
		case PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB_ALPHA_SRGB:
			return 16;
		default:
			return pigeon_image_format_bytes_per_pixel(f)*16;
	}
	return 0;
}

uint32_t pigeon_wgi_get_texture_size(unsigned int width, unsigned int height, 
	PigeonWGIImageFormat format, bool mip_maps)
{
	uint32_t sz = 0;
	uint32_t mip_sz = ((width*height)/16) * pigeon_image_format_bytes_per_4x4_block(format);

	if(!mip_maps) return mip_sz;

	while(width >= 4 && height >= 4) {
		sz += mip_sz;
		width /= 2;
		height /= 2;
		mip_sz /= 4;
	}

	return sz;
}
