#include "singleton.h"

#include <pigeon/assert.h>
#include <pigeon/wgi/textures.h>
#include <pigeon/wgi/vulkan/command.h>
#include <pigeon/wgi/vulkan/descriptor.h>
#include <pigeon/wgi/vulkan/fence.h>
#include <pigeon/wgi/vulkan/image.h>
#include <pigeon/wgi/vulkan/memory.h>
#include <pigeon/wgi/vulkan/sampler.h>
#include <stdlib.h>
#include <string.h>

PIGEON_ERR_RET pigeon_wgi_create_descriptor_layouts(void)
{
	PigeonVulkanDescriptorBinding bindings[6];

	/* depth */

	memset(bindings, 0, sizeof bindings);

	// Per-frame data
	bindings[0].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_UNIFORM;
	bindings[0].vertex_shader_accessible = true;
	bindings[0].elements = 1;

	// Per-draw-call data
	bindings[1].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_SSBO;
	bindings[1].vertex_shader_accessible = true;
	bindings[1].elements = 1;

	// Bone matrices
	bindings[2].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_SSBO;
	bindings[2].vertex_shader_accessible = true;
	bindings[2].elements = 1;

	ASSERT_R1(!pigeon_vulkan_create_descriptor_layout(&singleton_data.depth_descriptor_layout, 3, bindings));

	/* 1 texture */

	memset(bindings, 0, sizeof bindings);

	bindings[0].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[0].fragment_shader_accessible = true;
	bindings[0].elements = 1;

	ASSERT_R1(!pigeon_vulkan_create_descriptor_layout(&singleton_data.one_texture_descriptor_layout, 1, bindings));

	/* 2 textures */

	bindings[1].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[1].fragment_shader_accessible = true;
	bindings[1].elements = 1;

	ASSERT_R1(!pigeon_vulkan_create_descriptor_layout(&singleton_data.two_texture_descriptor_layout, 2, bindings));

	/* post */

	bindings[1].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[1].fragment_shader_accessible = true;
	bindings[1].elements = 1;

	ASSERT_R1(!pigeon_vulkan_create_descriptor_layout(&singleton_data.post_descriptor_layout, 2, bindings));

	/* light pass and render */

	memset(bindings, 0, sizeof bindings);

	// Per-scene data
	bindings[0].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_UNIFORM;
	bindings[0].vertex_shader_accessible = true;
	bindings[0].fragment_shader_accessible = true;
	bindings[0].elements = 1;

	// Per-draw-call data
	bindings[1].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_SSBO;
	bindings[1].vertex_shader_accessible = true;
	bindings[1].fragment_shader_accessible = true;
	bindings[1].elements = 1;

	// Bone matrices
	bindings[2].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_SSBO;
	bindings[2].vertex_shader_accessible = true;
	bindings[2].elements = 1;

	// Depth texture or blurred light
	bindings[3].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[3].fragment_shader_accessible = true;
	bindings[3].elements = 1;

	// Shadow map textures
	bindings[4].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[4].fragment_shader_accessible = true;
	bindings[4].elements = 4;

	// Textures
	bindings[5].type = PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE;
	bindings[5].fragment_shader_accessible = true;
	bindings[5].elements = 59;

	ASSERT_R1(!pigeon_vulkan_create_descriptor_layout(&singleton_data.render_descriptor_layout, 6, bindings));

	return 0;
}

void pigeon_wgi_destroy_descriptor_layouts(void)
{
	if (singleton_data.render_descriptor_layout.handle)
		pigeon_vulkan_destroy_descriptor_layout(&singleton_data.render_descriptor_layout);
	if (singleton_data.depth_descriptor_layout.handle)
		pigeon_vulkan_destroy_descriptor_layout(&singleton_data.depth_descriptor_layout);
	if (singleton_data.one_texture_descriptor_layout.handle)
		pigeon_vulkan_destroy_descriptor_layout(&singleton_data.one_texture_descriptor_layout);
	if (singleton_data.two_texture_descriptor_layout.handle)
		pigeon_vulkan_destroy_descriptor_layout(&singleton_data.two_texture_descriptor_layout);
	if (singleton_data.post_descriptor_layout.handle)
		pigeon_vulkan_destroy_descriptor_layout(&singleton_data.post_descriptor_layout);
}

PIGEON_ERR_RET pigeon_wgi_create_samplers(void)
{
	ASSERT_R1(!pigeon_vulkan_create_sampler(&singleton_data.nearest_filter_sampler, false, false, false, true, false));
	ASSERT_R1(!pigeon_vulkan_create_sampler(&singleton_data.bilinear_sampler, true, false, false, true, false));
	ASSERT_R1(!pigeon_vulkan_create_sampler(&singleton_data.shadow_sampler, false, false, true, true, false));
	ASSERT_R1(!pigeon_vulkan_create_sampler(&singleton_data.texture_sampler, true, true, true, false, true));
	return 0;
}

void pigeon_wgi_destroy_samplers(void)
{
	if (singleton_data.nearest_filter_sampler.vk_sampler)
		pigeon_vulkan_destroy_sampler(&singleton_data.nearest_filter_sampler);
	if (singleton_data.bilinear_sampler.vk_sampler)
		pigeon_vulkan_destroy_sampler(&singleton_data.bilinear_sampler);
	if (singleton_data.shadow_sampler.vk_sampler)
		pigeon_vulkan_destroy_sampler(&singleton_data.shadow_sampler);
	if (singleton_data.texture_sampler.vk_sampler)
		pigeon_vulkan_destroy_sampler(&singleton_data.texture_sampler);
}

static PIGEON_ERR_RET create_default_image_objects(void)
{
	PigeonVulkanMemoryRequirements memory_req_white;
	PigeonVulkanMemoryRequirements memory_req_black;
	PigeonVulkanMemoryRequirements memory_req_shadow;

	if (pigeon_vulkan_create_image(&singleton_data.default_1px_white_texture_image,
			PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR, 1, 1, 1, 1, false, false, true, false, false, true,
			&memory_req_white))
		return 1;

	if (pigeon_vulkan_create_image(&singleton_data.default_1px_black_texture_image,
			PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR, 1, 1, 1, 1, false, false, true, false, false, true,
			&memory_req_black))
		return 1;

	if (pigeon_vulkan_create_image(&singleton_data.default_shadow_map_image, PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32, 1, 1, 1,
			1, false, false, true, false, false, true, &memory_req_shadow))
		return 1;

	PigeonVulkanMemoryTypePreferences preferences = { 0 };
	preferences.device_local = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED;
	preferences.host_visible = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT;
	preferences.host_coherent = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT;
	preferences.host_cached = PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT;

	// TODO merge into 1 allocation
	// TODO merge white & black 1px textures into 1 image & 2 image views

	if (pigeon_vulkan_allocate_memory(&singleton_data.default_textures_memory, memory_req_white, preferences))
		return 1;
	if (pigeon_vulkan_allocate_memory(&singleton_data.default_textures_memory_black, memory_req_black, preferences))
		return 1;
	if (pigeon_vulkan_allocate_memory(&singleton_data.default_shadow_map_memory, memory_req_shadow, preferences))
		return 1;

	if (pigeon_vulkan_image_bind_memory(
			&singleton_data.default_1px_white_texture_image, &singleton_data.default_textures_memory, 0))
		return 1;
	if (pigeon_vulkan_image_bind_memory(
			&singleton_data.default_1px_black_texture_image, &singleton_data.default_textures_memory_black, 0))
		return 1;
	if (pigeon_vulkan_image_bind_memory(
			&singleton_data.default_shadow_map_image, &singleton_data.default_shadow_map_memory, 0))
		return 1;

	if (pigeon_vulkan_create_image_view(&singleton_data.default_1px_white_texture_image_view,
			&singleton_data.default_1px_white_texture_image, false))
		return 1;
	if (pigeon_vulkan_create_image_view(&singleton_data.default_1px_white_texture_array_image_view,
			&singleton_data.default_1px_white_texture_image, true))
		return 1;

	if (pigeon_vulkan_create_image_view(&singleton_data.default_1px_black_texture_image_view,
			&singleton_data.default_1px_black_texture_image, false))
		return 1;
	if (pigeon_vulkan_create_image_view(&singleton_data.default_1px_black_texture_array_image_view,
			&singleton_data.default_1px_black_texture_image, true))
		return 1;

	if (pigeon_vulkan_create_image_view(
			&singleton_data.default_shadow_map_image_view, &singleton_data.default_shadow_map_image, false))
		return 1;

	return 0;
}

static PIGEON_ERR_RET transition_default_images()
{
	PigeonVulkanCommandPool command_pool;
	if (pigeon_vulkan_create_command_pool(&command_pool, 1, true, false, true))
		return 1;

	PigeonVulkanFence fence;

	if (pigeon_vulkan_create_fence(&fence, false)) {
		pigeon_vulkan_destroy_command_pool(&command_pool);
		return 1;
	}

#define CLEANUP()                                                                                                      \
	{                                                                                                                  \
		pigeon_vulkan_destroy_fence(&fence);                                                                           \
		pigeon_vulkan_destroy_command_pool(&command_pool);                                                             \
	}

	if (pigeon_vulkan_start_submission(&command_pool, 0)) {
		CLEANUP();
		return 1;
	}

	pigeon_vulkan_transition_image_to_transfer_dst(&command_pool, 0, &singleton_data.default_1px_white_texture_image);
	pigeon_vulkan_clear_image(
		&command_pool, 0, &singleton_data.default_1px_white_texture_image, 1.0f, 1.0f, 1.0f, 1.0f);
	pigeon_vulkan_transition_transfer_dst_to_shader_read(
		&command_pool, 0, &singleton_data.default_1px_white_texture_image);

	pigeon_vulkan_transition_image_to_transfer_dst(&command_pool, 0, &singleton_data.default_1px_black_texture_image);
	pigeon_vulkan_clear_image(
		&command_pool, 0, &singleton_data.default_1px_black_texture_image, 0.0f, 0.0f, 0.0f, 1.0f);
	pigeon_vulkan_transition_transfer_dst_to_shader_read(
		&command_pool, 0, &singleton_data.default_1px_black_texture_image);

	pigeon_vulkan_transition_image_to_transfer_dst(&command_pool, 0, &singleton_data.default_shadow_map_image);
	pigeon_vulkan_clear_depth_image(&command_pool, 0, &singleton_data.default_shadow_map_image, 0.0f);
	pigeon_vulkan_transition_transfer_dst_to_shader_read(&command_pool, 0, &singleton_data.default_shadow_map_image);

	if (pigeon_vulkan_submit(&command_pool, 0, &fence)) {
		CLEANUP();
		return 1;
	}

	int err = pigeon_vulkan_wait_fence(&fence);

	CLEANUP();

	if (err)
		return 1;

#undef CLEANUP

	return 0;
}

static PIGEON_ERR_RET create_default_image_objects_gl(void)
{

	ASSERT_R1(!pigeon_opengl_create_texture(
		&singleton_data.gl.default_1px_white_texture_image, PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR, 1, 1, 0, 1));

	uint8_t pixels[4] = { 255, 255, 255, 255 };
	ASSERT_R1(!pigeon_opengl_upload_texture_mip(&singleton_data.gl.default_1px_white_texture_image, 0, 0, pixels, 4));

	ASSERT_R1(!pigeon_opengl_create_texture(
		&singleton_data.gl.default_1px_black_texture_image, PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR, 1, 1, 0, 1));

	pixels[0] = pixels[1] = pixels[2] = 0;
	ASSERT_R1(!pigeon_opengl_upload_texture_mip(&singleton_data.gl.default_1px_black_texture_image, 0, 0, pixels, 4));

	ASSERT_R1(!pigeon_opengl_create_texture(
		&singleton_data.gl.default_shadow_map_image, PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32, 1, 1, 0, 1));

	pigeon_opengl_set_texture_sampler(&singleton_data.gl.default_shadow_map_image, true, false, true, false, false);

	float depth = 0;
	ASSERT_R1(!pigeon_opengl_upload_texture_mip(&singleton_data.gl.default_shadow_map_image, 0, 0, &depth, 4));

	return 0;
}

PIGEON_ERR_RET pigeon_wgi_create_default_textures(void)
{
	if (VULKAN && create_default_image_objects())
		return 1;
	if (OPENGL && create_default_image_objects_gl())
		return 1;
	if (VULKAN && transition_default_images())
		return 1;
	return 0;
}

void pigeon_wgi_destroy_default_textures(void)
{
	if (VULKAN) {
		pigeon_vulkan_destroy_image_view(&singleton_data.default_1px_white_texture_image_view);
		pigeon_vulkan_destroy_image_view(&singleton_data.default_1px_white_texture_array_image_view);
		pigeon_vulkan_destroy_image(&singleton_data.default_1px_white_texture_image);
		pigeon_vulkan_free_memory(&singleton_data.default_textures_memory);

		pigeon_vulkan_destroy_image_view(&singleton_data.default_1px_black_texture_image_view);
		pigeon_vulkan_destroy_image_view(&singleton_data.default_1px_black_texture_array_image_view);
		pigeon_vulkan_destroy_image(&singleton_data.default_1px_black_texture_image);
		pigeon_vulkan_free_memory(&singleton_data.default_textures_memory_black);

		pigeon_vulkan_destroy_image_view(&singleton_data.default_shadow_map_image_view);
		pigeon_vulkan_destroy_image(&singleton_data.default_shadow_map_image);
		pigeon_vulkan_free_memory(&singleton_data.default_shadow_map_memory);
	} else {
		pigeon_opengl_destroy_texture(&singleton_data.gl.default_1px_white_texture_image);
		pigeon_opengl_destroy_texture(&singleton_data.gl.default_1px_black_texture_image);
		pigeon_opengl_destroy_texture(&singleton_data.gl.default_shadow_map_image);
	}
}

PIGEON_ERR_RET pigeon_wgi_create_descriptor_pools(void)
{
	if (singleton_data.full_render_cfg.ssao) {
		for (unsigned int i = 0; i < 4; i++) {
			if (pigeon_vulkan_create_descriptor_pool(
					&singleton_data.ssao_descriptor_pools[i], 1, &singleton_data.one_texture_descriptor_layout))
				return 1;
		}
	}

	if (singleton_data.full_render_cfg.bloom) {
		if (pigeon_vulkan_create_descriptor_pool(
				&singleton_data.bloom_downscale_descriptor_pool, 1, &singleton_data.one_texture_descriptor_layout))
			return 1;
		for (unsigned int i = 0; i < 3; i++) {
			for (unsigned int j = 0; j < 2; j++) {
				if (pigeon_vulkan_create_descriptor_pool(
						&singleton_data.bloom_descriptor_pools[i][j], 1, &singleton_data.one_texture_descriptor_layout))
					return 1;
			}
		}
		if (pigeon_vulkan_create_descriptor_pool(
				&singleton_data.bloom_blur_merge_descriptor_pool0, 1, &singleton_data.two_texture_descriptor_layout))
			return 1;
		if (pigeon_vulkan_create_descriptor_pool(
				&singleton_data.bloom_blur_merge_descriptor_pool1, 1, &singleton_data.two_texture_descriptor_layout))
			return 1;
	}

	if (singleton_data.full_render_cfg.bloom) {
		if (pigeon_vulkan_create_descriptor_pool(
				&singleton_data.post_process_descriptor_pool, 1, &singleton_data.post_descriptor_layout))
			return 1;
	}
	if (pigeon_vulkan_create_descriptor_pool(
			&singleton_data.post_process_descriptor_pool_no_bloom, 1, &singleton_data.post_descriptor_layout))
		return 1;

	return 0;
}

void pigeon_wgi_set_global_descriptors(void)
{

	if (singleton_data.full_render_cfg.ssao) {
		pigeon_vulkan_set_descriptor_texture(&singleton_data.ssao_descriptor_pools[0], 0, 0, 0,
			&singleton_data.depth_image.image_view, &singleton_data.nearest_filter_sampler);

		for (unsigned int i = 0; i < 3; i++) {
			pigeon_vulkan_set_descriptor_texture(&singleton_data.ssao_descriptor_pools[i + 1], 0, 0, 0,
				&singleton_data.ssao_images[i].image_view, &singleton_data.bilinear_sampler);
		}
	}

	if (singleton_data.full_render_cfg.bloom) {
		pigeon_vulkan_set_descriptor_texture(&singleton_data.bloom_downscale_descriptor_pool, 0, 0, 0,
			&singleton_data.render_image.image_view, &singleton_data.bilinear_sampler);

		for (unsigned int i = 0; i < 3; i++) {
			for (unsigned int j = 0; j < 2; j++) {
				pigeon_vulkan_set_descriptor_texture(&singleton_data.bloom_descriptor_pools[i][j], 0, 0, 0,
					&singleton_data.bloom_images[i][j].image_view, &singleton_data.bilinear_sampler);
			}
		}
		pigeon_vulkan_set_descriptor_texture(&singleton_data.bloom_blur_merge_descriptor_pool0, 0, 0, 0,
			&singleton_data.bloom_images[1][1].image_view, &singleton_data.bilinear_sampler);
		pigeon_vulkan_set_descriptor_texture(&singleton_data.bloom_blur_merge_descriptor_pool0, 0, 1, 0,
			&singleton_data.bloom_images[2][0].image_view, &singleton_data.bilinear_sampler);

		pigeon_vulkan_set_descriptor_texture(&singleton_data.bloom_blur_merge_descriptor_pool1, 0, 0, 0,
			&singleton_data.bloom_images[0][1].image_view, &singleton_data.bilinear_sampler);
		pigeon_vulkan_set_descriptor_texture(&singleton_data.bloom_blur_merge_descriptor_pool1, 0, 1, 0,
			&singleton_data.bloom_images[1][0].image_view, &singleton_data.bilinear_sampler);
	}

	pigeon_vulkan_set_descriptor_texture(&singleton_data.post_process_descriptor_pool_no_bloom, 0, 0, 0,
		&singleton_data.render_image.image_view, &singleton_data.bilinear_sampler);
	pigeon_vulkan_set_descriptor_texture(&singleton_data.post_process_descriptor_pool_no_bloom, 0, 1, 0,
		&singleton_data.default_1px_black_texture_image_view, &singleton_data.bilinear_sampler);

	if (singleton_data.full_render_cfg.bloom) {
		pigeon_vulkan_set_descriptor_texture(&singleton_data.post_process_descriptor_pool, 0, 0, 0,
			&singleton_data.render_image.image_view, &singleton_data.bilinear_sampler);
		pigeon_vulkan_set_descriptor_texture(&singleton_data.post_process_descriptor_pool, 0, 1, 0,
			&singleton_data.bloom_images[0][0].image_view, &singleton_data.bilinear_sampler);
	}
}

void pigeon_wgi_destroy_descriptor_pools(void)
{
	for (unsigned int i = 0; i < 4; i++) {
		pigeon_vulkan_destroy_descriptor_pool(&singleton_data.ssao_descriptor_pools[i]);
	}
	pigeon_vulkan_destroy_descriptor_pool(&singleton_data.bloom_downscale_descriptor_pool);
	for (unsigned int i = 0; i < 3; i++) {
		for (unsigned int j = 0; j < 2; j++) {
			pigeon_vulkan_destroy_descriptor_pool(&singleton_data.bloom_descriptor_pools[i][j]);
		}
	}
	pigeon_vulkan_destroy_descriptor_pool(&singleton_data.bloom_blur_merge_descriptor_pool0);
	pigeon_vulkan_destroy_descriptor_pool(&singleton_data.bloom_blur_merge_descriptor_pool1);
	pigeon_vulkan_destroy_descriptor_pool(&singleton_data.post_process_descriptor_pool);
	pigeon_vulkan_destroy_descriptor_pool(&singleton_data.post_process_descriptor_pool_no_bloom);
}

unsigned int pigeon_image_format_bytes_per_pixel(PigeonWGIImageFormat f)
{
	switch (f) {
	case PIGEON_WGI_IMAGE_FORMAT_NONE:
		return 0;
	case PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR:
		return 1;
	case PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR:
	case PIGEON_WGI_IMAGE_FORMAT_DEPTH_U16:
		return 2;
	case PIGEON_WGI_IMAGE_FORMAT_DEPTH_U24:
		return 3;
	case PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR:
	case PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB:
	case PIGEON_WGI_IMAGE_FORMAT_BGRA_U8_SRGB:
	case PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32:
	case PIGEON_WGI_IMAGE_FORMAT_B10G11R11_UF_LINEAR:
	case PIGEON_WGI_IMAGE_FORMAT_RG_U16_LINEAR:
	case PIGEON_WGI_IMAGE_FORMAT_A2B10G10R10_LINEAR:
		return 4;
	case PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR:
		return 8;
	default:;
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
	case PIGEON_WGI_IMAGE_FORMAT_BC5:
	case PIGEON_WGI_IMAGE_FORMAT_BC7_SRGB:
	case PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB_ALPHA_SRGB:
		return 16;
	default:
		return pigeon_image_format_bytes_per_pixel(f) * 16;
	}
	return 0;
}

uint32_t pigeon_wgi_get_texture_size(
	unsigned int width, unsigned int height, PigeonWGIImageFormat format, bool mip_maps)
{
	uint32_t sz = 0;
	uint32_t mip_sz = ((width * height) / 16) * pigeon_image_format_bytes_per_4x4_block(format);

	if (!mip_maps)
		return mip_sz;

	while (width >= 4 && height >= 4) {
		sz += mip_sz;
		width /= 2;
		height /= 2;
		mip_sz /= 4;
	}

	return sz;
}
