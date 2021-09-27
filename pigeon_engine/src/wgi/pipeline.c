#include <stdint.h>
#include "singleton.h"
#include <pigeon/wgi/pipeline.h>
#include <pigeon/wgi/vulkan/renderpass.h>
#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/wgi/vulkan/vulkan.h>
#include <pigeon/util.h>

ERROR_RETURN_TYPE pigeon_wgi_create_render_passes(void)
{
	PigeonVulkanRenderPassConfig config;

	// depth prepass & shadow maps
	memset(&config, 0, sizeof config);
	config.vertex_shader_depends_on_transfer = true;
	config.depth_mode = PIGEON_VULKAN_RENDER_PASS_DEPTH_KEEP;
	config.depth_format = PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32;

	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_depth, config)) return 1;



	// light pass
	memset(&config, 0, sizeof config);
	config.depth_mode = PIGEON_VULKAN_RENDER_PASS_DEPTH_READ_ONLY;
	config.depth_format = PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32;
	config.colour_image = singleton_data.light_framebuffer_image_format;
	config.clear_colour_image = true;

	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_light_pass, config)) return 1;



	// light blur
	memset(&config, 0, sizeof config);
	config.colour_image = singleton_data.light_framebuffer_image_format;
	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_light_blur, config)) return 1;


    PigeonWGIImageFormat hdr_format = pigeon_vulkan_compact_hdr_framebuffer_available() ?
    	PIGEON_WGI_IMAGE_FORMAT_B10G11R11_UF_LINEAR : PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR;

	// bloom blur
	memset(&config, 0, sizeof config);
	config.fragment_shader_depends_on_transfer = true;
	config.colour_image = hdr_format;
	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_bloom_blur, config)) return 1;

	// render
	memset(&config, 0, sizeof config);
	config.depth_mode = PIGEON_VULKAN_RENDER_PASS_DEPTH_READ_ONLY;
	config.depth_format = PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32;
	config.colour_image = hdr_format;
	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_render, config)) return 1;

	// post
	memset(&config, 0, sizeof config);
    PigeonVulkanSwapchainInfo sc_info = pigeon_vulkan_get_swapchain_info();
	config.colour_image = sc_info.format;
	config.colour_image_is_swapchain = true;
	if (pigeon_vulkan_make_render_pass(&singleton_data.rp_post, config)) return 1;

	return 0;

}

void pigeon_wgi_destroy_render_passes(void)
{
	if (singleton_data.rp_depth.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_depth);
	if (singleton_data.rp_light_pass.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_light_pass);
	if (singleton_data.rp_light_blur.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_light_blur);
	if (singleton_data.rp_bloom_blur.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_bloom_blur);
	if (singleton_data.rp_render.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_render);
	if (singleton_data.rp_post.vk_renderpass) pigeon_vulkan_destroy_render_pass(&singleton_data.rp_post);

}

static int create_pipeine(PigeonVulkanPipeline * pipeline, const char * vs_path, const char * fs_path, 
	PigeonVulkanRenderPass* render_pass, PigeonVulkanDescriptorLayout * descriptor_layout,
	unsigned int push_constant_size, unsigned int specialisation_constants, uint32_t * sc_data)
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
		descriptor_layout, &config, specialisation_constants, sc_data);
	pigeon_vulkan_destroy_shader(&vs);
	pigeon_vulkan_destroy_shader(&fs);
	if (err) {
		return 1;
	}

	return 0;

}

ERROR_RETURN_TYPE pigeon_wgi_create_standard_pipeline_objects(void)
{
#ifdef NDEBUG
	#define SHADER_PATH(x) ("build/release/standard_assets/shaders/" x ".spv")
#else
	#define SHADER_PATH(x) ("build/debug/standard_assets/shaders/" x ".spv")
#endif


	if (create_pipeine(&singleton_data.pipeline_blur, SHADER_PATH("gaussian.vert"), SHADER_PATH("gaussian_rgb.frag"),
		&singleton_data.rp_bloom_blur, &singleton_data.one_texture_descriptor_layout, 16, 0, NULL)) ASSERT_1(false);

	const char * gaussian_light_frag_path = NULL;
	if(singleton_data.light_image_components == 1) {
		gaussian_light_frag_path = SHADER_PATH("gaussian_light.frag.1");
	}
	else if(singleton_data.light_image_components == 2) {
		gaussian_light_frag_path = SHADER_PATH("gaussian_light.frag");
	}
	else if(singleton_data.light_image_components == 3) {
		gaussian_light_frag_path = SHADER_PATH("gaussian_light.frag.3");
	}

	if (gaussian_light_frag_path && 
		create_pipeine(&singleton_data.pipeline_light_blur, SHADER_PATH("gaussian.vert"), gaussian_light_frag_path,
		&singleton_data.rp_light_blur, &singleton_data.two_texture_descriptor_layout, 16, 0, NULL)) ASSERT_1(false);

	

	uint32_t sc_bloom_samples = 4;

	if (create_pipeine(&singleton_data.pipeline_bloom_downsample,
		SHADER_PATH("downsample.vert"), SHADER_PATH("downsample.frag"),
		&singleton_data.rp_bloom_blur, &singleton_data.one_texture_descriptor_layout, 12, 1, &sc_bloom_samples)) return 1;
	
		
	uint32_t sc_use_bloom = singleton_data.render_graph.bloom ? 1 : 0;

	if (create_pipeine(&singleton_data.pipeline_post, SHADER_PATH("post.vert"), SHADER_PATH("post.frag"),
		&singleton_data.rp_post, &singleton_data.post_descriptor_layout, 20, 1, &sc_use_bloom)) return 1;

#undef SHADER_PATH
	return 0;
}

void pigeon_wgi_destroy_standard_pipeline_objects()
{
	if (singleton_data.pipeline_blur.vk_pipeline) pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_blur);
	if (singleton_data.pipeline_bloom_downsample.vk_pipeline) pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_bloom_downsample);
	if (singleton_data.pipeline_light_blur.vk_pipeline) pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_light_blur);
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


int pigeon_wgi_create_skybox_pipeline(PigeonWGIPipeline* pipeline, PigeonWGIShader* vs, PigeonWGIShader* fs)
{
	ASSERT_1(pipeline && vs && fs);

	PigeonWGIPipelineConfig config = {0};
	config.depth_test = true;

	pipeline->pipeline = calloc(1, sizeof *pipeline->pipeline);
	ASSERT_1(pipeline->pipeline);

	if(pigeon_vulkan_create_pipeline(pipeline->pipeline, vs->shader, fs->shader,
		0, &singleton_data.rp_render, &singleton_data.render_descriptor_layout, &config, 0, NULL)) {
		free(pipeline->pipeline);
		ASSERT_1(false);
	}

	return 0;
}

int pigeon_wgi_create_pipeline(PigeonWGIPipeline* pipeline, 
	PigeonWGIShader* vs_depth, 
	PigeonWGIShader* vs_light, PigeonWGIShader* vs, 
	PigeonWGIShader* fs_depth, // NULL for opaque objects
	PigeonWGIShader* fs_light, PigeonWGIShader* fs,
	const PigeonWGIPipelineConfig* config)
{
	ASSERT_1(pipeline && vs_depth && vs_light && vs && fs && fs_light && config);

	pipeline->blend_enabled = config->blend_function != PIGEON_WGI_BLEND_NONE;

	pipeline->pipeline_depth = calloc(1, sizeof *pipeline->pipeline_depth);
	pipeline->pipeline_shadow_map = calloc(1, sizeof *pipeline->pipeline_shadow_map);
	pipeline->pipeline_light = calloc(1, sizeof *pipeline->pipeline_light);
	pipeline->pipeline = calloc(1, sizeof *pipeline->pipeline);

	if(!pipeline->pipeline_depth || !pipeline->pipeline_shadow_map || !pipeline->pipeline_light || !pipeline->pipeline) {
		if(pipeline->pipeline_depth) free(pipeline->pipeline_depth);
		if(pipeline->pipeline_shadow_map) free(pipeline->pipeline_shadow_map);
		if(pipeline->pipeline_light) free(pipeline->pipeline_light);
		if(pipeline->pipeline) free(pipeline->pipeline);
		ASSERT_1(false);
	}

	#define ERR() \
		free(pipeline->pipeline_depth); \
		free(pipeline->pipeline_shadow_map); \
		free(pipeline->pipeline_light); \
		free(pipeline->pipeline); \
		ASSERT_1(false);


	PigeonWGIPipelineConfig config2 = *config;
	config2.depth_write = false;
	config2.depth_test = true;

	uint32_t ssao_enabled = singleton_data.render_graph.ssao ? 1 : 0;
	if(pigeon_vulkan_create_pipeline(pipeline->pipeline, vs->shader, fs->shader,
		8, &singleton_data.rp_render, &singleton_data.render_descriptor_layout, &config2, 1, &ssao_enabled))
	{
		free(pipeline->pipeline);
		ERR();
	}

	

	uint32_t sc[4] = {singleton_data.render_graph.ssao ? 8 : 0, 4,
		config->blend_function == PIGEON_WGI_BLEND_NONE ? 0 : 1, 2};
	if(pigeon_vulkan_create_pipeline(pipeline->pipeline_light, vs_light->shader, fs_light->shader,
		8, &singleton_data.rp_light_pass, &singleton_data.light_pass_descriptor_layout, &config2, 4, sc))
	{
		pigeon_vulkan_destroy_pipeline(pipeline->pipeline);
		ERR();
	}

	config2.depth_write = true;
	config2.depth_test = true;
	config2.depth_only = true;

	if(pigeon_vulkan_create_pipeline(pipeline->pipeline_depth, vs_depth->shader, fs_depth ? fs_depth->shader : NULL,
		8, &singleton_data.rp_depth, 
		config2.blend_function == PIGEON_WGI_BLEND_NONE ?
			&singleton_data.depth_descriptor_layout : &singleton_data.render_descriptor_layout,
		&config2, 0, NULL))
	{
		pigeon_vulkan_destroy_pipeline(pipeline->pipeline);
		pigeon_vulkan_destroy_pipeline(pipeline->pipeline_light);
		ERR();
	}

	config2.depth_bias = true;

	if(pigeon_vulkan_create_pipeline(pipeline->pipeline_shadow_map, vs_depth->shader, fs_depth ? fs_depth->shader : NULL,
		8, &singleton_data.rp_depth,
		config2.blend_function == PIGEON_WGI_BLEND_NONE ?
			&singleton_data.depth_descriptor_layout : &singleton_data.render_descriptor_layout,
		&config2, 0, NULL))
	{
		pigeon_vulkan_destroy_pipeline(pipeline->pipeline);
		pigeon_vulkan_destroy_pipeline(pipeline->pipeline_light);
		pigeon_vulkan_destroy_pipeline(pipeline->pipeline_depth);
		ERR();
	}

	return 0;
}

void pigeon_wgi_destroy_pipeline(PigeonWGIPipeline* pipeline)
{
	if(pipeline && pipeline->pipeline) {
		pigeon_vulkan_destroy_pipeline(pipeline->pipeline);
		free(pipeline->pipeline);
		pipeline->pipeline = NULL;

		if(pipeline->pipeline_depth) {
			pigeon_vulkan_destroy_pipeline(pipeline->pipeline_depth);
			free(pipeline->pipeline_depth);
			pipeline->pipeline_depth = NULL;
		}
		if(pipeline->pipeline_shadow_map) {
			pigeon_vulkan_destroy_pipeline(pipeline->pipeline_shadow_map);
			free(pipeline->pipeline_shadow_map);
			pipeline->pipeline_shadow_map = NULL;
		}
		if(pipeline->pipeline_light) {
			pigeon_vulkan_destroy_pipeline(pipeline->pipeline_light);
			free(pipeline->pipeline_light);
			pipeline->pipeline_light = NULL;
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