#include <stdint.h>
#include "singleton.h"
#include <pigeon/wgi/pipeline.h>
#include <pigeon/wgi/vulkan/renderpass.h>
#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/util.h>

ERROR_RETURN_TYPE pigeon_wgi_create_render_passes(void)
{
	PigeonVulkanRenderPassConfig config = {0};

	// depth prepass
	config.colour_image = PIGEON_WGI_IMAGE_FORMAT_NONE;
	config.vertex_shader_depends_on_transfer = true;
	config.fragment_shader_depends_on_transfer = false;
	config.depth_mode = PIGEON_VULKAN_RENDER_PASS_DEPTH_KEEP;
	config.depth_format = PIGEON_WGI_IMAGE_FORMAT_DEPTH_U16;

	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_depth, config)) return 1;

	// ssao, blur
	config.vertex_shader_depends_on_transfer = false;
	config.depth_mode = PIGEON_VULKAN_RENDER_PASS_DEPTH_NONE;
	config.colour_image = PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR;
	config.colour_image_is_swapchain = false;
	config.clear_colour_image = false;

	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_ssao, config)) return 1;
	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_ssao_blur, config)) return 1; // TODO merge into 1

	config.colour_image = PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR;
	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_bloom_gaussian, config)) return 1;

	// render
	config.fragment_shader_depends_on_transfer = true;
	config.depth_mode = PIGEON_VULKAN_RENDER_PASS_DEPTH_READ_ONLY;
	config.colour_image = PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR;
	config.clear_colour_image = false;

	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_render, config)) return 1;

	// post
	config.depth_mode = PIGEON_VULKAN_RENDER_PASS_DEPTH_NONE;
    PigeonVulkanSwapchainInfo sc_info = pigeon_vulkan_get_swapchain_info();
	config.colour_image = sc_info.format;
	config.colour_image_is_swapchain = true;
	config.clear_colour_image = false;

	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_post, config)) return 1;

	return 0;

}

void pigeon_wgi_destroy_render_passes(void)
{
	if (singleton_data.rp_depth.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_depth);
	if (singleton_data.rp_ssao.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_ssao);
	if (singleton_data.rp_ssao_blur.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_ssao_blur);
	if (singleton_data.rp_bloom_gaussian.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_ssao_blur);
	if (singleton_data.rp_render.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_render);
	if (singleton_data.rp_post.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_post);

}

static int create_pipeine(PigeonVulkanPipeline * pipeline, const char * vs_path, const char * fs_path, 
	PigeonVulkanRenderPass* render_pass, PigeonVulkanDescriptorLayout * descriptor_layout,
	unsigned int push_constant_size)
{
	if (!pipeline || !vs_path || !fs_path || !render_pass | !descriptor_layout || push_constant_size > 128) {
		assert(false);
		return 1;
	}


	PigeonVulkanShader vs = { 0 }, fs = { 0 };
	if (pigeon_vulkan_load_shader(&vs, vs_path)) return 1;
	if (pigeon_vulkan_load_shader(&fs, fs_path)) {
		pigeon_vulkan_destroy_shader(&vs);
		return 1;
	}

	PigeonWGIPipelineConfig config = { 0 };


	int err = pigeon_vulkan_create_pipeline(pipeline, &vs, &fs, push_constant_size, render_pass, 
		descriptor_layout, &config);
	pigeon_vulkan_destroy_shader(&vs);
	pigeon_vulkan_destroy_shader(&fs);
	if (err) {
		return 1;
	}

	return 0;

}

ERROR_RETURN_TYPE pigeon_wgi_create_standard_pipeline_objects()
{
#ifdef NDEBUG
	#define SHADER_PATH(x) ("build/release/standard_assets/shaders/" x ".spv")
#else
	#define SHADER_PATH(x) ("build/debug/standard_assets/shaders/" x ".spv")
#endif

	if (create_pipeine(&singleton_data.pipeline_ssao, SHADER_PATH("ssao.vert"), SHADER_PATH("ssao.frag"),
		&singleton_data.rp_ssao, &singleton_data.one_texture_descriptor_layout, 20)) return 1;

	if (create_pipeine(&singleton_data.pipeline_ssao_blur, SHADER_PATH("gaussian.vert"), SHADER_PATH("ssao_blur.frag"),
		&singleton_data.rp_ssao_blur, &singleton_data.one_texture_descriptor_layout, 12)) return 1;

	if (create_pipeine(&singleton_data.pipeline_ssao_blur2, SHADER_PATH("gaussian.vert"), SHADER_PATH("ssao_blur2.frag"),
		&singleton_data.rp_ssao_blur, &singleton_data.one_texture_descriptor_layout, 12)) return 1;

	if (create_pipeine(&singleton_data.pipeline_bloom_gaussian, SHADER_PATH("gaussian.vert"), SHADER_PATH("bloom_gaussian.frag"),
		&singleton_data.rp_bloom_gaussian, &singleton_data.one_texture_descriptor_layout, 12)) return 1;

	if (create_pipeine(&singleton_data.pipeline_downsample, SHADER_PATH("downsample.vert"), 
		SHADER_PATH("downsample.frag"),
		&singleton_data.rp_bloom_gaussian, &singleton_data.one_texture_descriptor_layout, 12)) return 1;
		
	if (create_pipeine(&singleton_data.pipeline_post, SHADER_PATH("post.vert"), SHADER_PATH("post.frag"),
		&singleton_data.rp_post, &singleton_data.post_descriptor_layout, 20)) return 1;

#undef SHADER_PATH
	return 0;
}

void pigeon_wgi_destroy_standard_pipeline_objects()
{
	if (singleton_data.pipeline_ssao.vk_pipeline) pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_ssao);
	if (singleton_data.pipeline_ssao_blur.vk_pipeline) pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_ssao_blur);
	if (singleton_data.pipeline_ssao_blur2.vk_pipeline) pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_ssao_blur2);
	if (singleton_data.pipeline_downsample.vk_pipeline) pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_downsample);
	if (singleton_data.pipeline_bloom_gaussian.vk_pipeline) pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_bloom_gaussian);
	if (singleton_data.pipeline_post.vk_pipeline) pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_post);
}

ERROR_RETURN_TYPE pigeon_wgi_create_shader(PigeonWGIShader* shader, const void* spv, uint32_t spv_size, PigeonWGIShaderType type)
{
	(void) type;
	ASSERT_1(shader && spv && spv_size);

	shader->shader = malloc(sizeof shader->shader);

	if(pigeon_vulkan_create_shader(shader->shader, spv, spv_size)) {
		free(shader->shader);
		return 1;
	}

	return 0;
}

void pigeon_wgi_destroy_shader(PigeonWGIShader* shader)
{
	if(shader && shader->shader) {
		pigeon_vulkan_destroy_shader(shader->shader);
		free(shader->shader);
		shader->shader = NULL;
	}
	else {
		assert(false);
	}
}

int pigeon_wgi_create_pipeline(PigeonWGIPipeline* pipeline, PigeonWGIShader* vs, PigeonWGIShader* vs_depth_only, 
	PigeonWGIShader* fs, const PigeonWGIPipelineConfig* config)
{
	ASSERT_1(pipeline && vs && fs && config);

	pipeline->pipeline = calloc(1, sizeof *pipeline->pipeline);
	ASSERT_1(pipeline->pipeline);


	if(pigeon_vulkan_create_pipeline(pipeline->pipeline, vs->shader, fs ? fs->shader : NULL,
			4, &singleton_data.rp_render, &singleton_data.render_descriptor_layout, config)) {
		free(pipeline->pipeline);
		pipeline->pipeline = NULL;
	}

	if(vs_depth_only) {
		pipeline->pipeline_depth_only = calloc(1, sizeof *pipeline->pipeline_depth_only);
		if(!pipeline->pipeline_depth_only) {
			pigeon_vulkan_destroy_pipeline(pipeline->pipeline);
			free(pipeline->pipeline);
			pipeline->pipeline = NULL;
			ASSERT_1(false);
		}

		PigeonWGIPipelineConfig config2 = *config;
		config2.depth_write = true;
		config2.depth_test = true;
		config2.depth_only = true;
		if(pigeon_vulkan_create_pipeline(pipeline->pipeline_depth_only, vs_depth_only->shader, NULL,
				4, &singleton_data.rp_depth, &singleton_data.depth_descriptor_layout, &config2)) {
			pigeon_vulkan_destroy_pipeline(pipeline->pipeline);
			free(pipeline->pipeline);
			pipeline->pipeline = NULL;
			free(pipeline->pipeline_depth_only);
			pipeline->pipeline_depth_only = NULL;
		}
	}

	pipeline->has_fragment_shader = fs != NULL;
	return 0;
}

void pigeon_wgi_destroy_pipeline(PigeonWGIPipeline* pipeline)
{
	if(pipeline && pipeline->pipeline) {
		pigeon_vulkan_destroy_pipeline(pipeline->pipeline);
		free(pipeline->pipeline);
		pipeline->pipeline = NULL;
		if(pipeline->pipeline_depth_only) {
			pigeon_vulkan_destroy_pipeline(pipeline->pipeline_depth_only);
			free(pipeline->pipeline_depth_only);
			pipeline->pipeline_depth_only = NULL;
		}
	}
	else {
		assert(false);
	}
}

uint32_t pigeon_wgi_get_vertex_attribute_type_size(PigeonWGIVertexAttributeType type)
{
	switch(type) {
		case PIGEON_WGI_VERTEX_ATTRIBUTE_NONE:
			return 0;
		case PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION:
		case PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR:
			return 3*4;
		case PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION2D:
		case PIGEON_WGI_VERTEX_ATTRIBUTE_UV_FLOAT:
			return 2*4;
		case PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED:
		case PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR_RGBA8:
		case PIGEON_WGI_VERTEX_ATTRIBUTE_NORMAL:
		case PIGEON_WGI_VERTEX_ATTRIBUTE_TANGENT:
		case PIGEON_WGI_VERTEX_ATTRIBUTE_UV:
		case PIGEON_WGI_VERTEX_ATTRIBUTE_BONE2:
			return 4;
	}
	return 0;
}
