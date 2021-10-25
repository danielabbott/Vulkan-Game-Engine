#include <stdint.h>
#include "singleton.h"
#include <pigeon/wgi/pipeline.h>
#include <pigeon/wgi/vulkan/renderpass.h>
#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/wgi/vulkan/vulkan.h>
#include <pigeon/wgi/opengl/shader.h>
#include <pigeon/assert.h>
#include <pigeon/misc.h>
#include <string.h>

PIGEON_ERR_RET pigeon_wgi_create_render_passes(void)
{
	PigeonVulkanRenderPassConfig config;

	// depth prepass & shadow maps
	memset(&config, 0, sizeof config);
	config.depth_mode = PIGEON_VULKAN_RENDER_PASS_DEPTH_KEEP;
	config.depth_format = PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32;

	ASSERT_R1 (!pigeon_vulkan_make_render_pass(&singleton_data.rp_depth, config));



	// light pass
	memset(&config, 0, sizeof config);
	config.depth_mode = PIGEON_VULKAN_RENDER_PASS_DEPTH_READ_ONLY;
	config.depth_format = PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32;
	config.colour_image = singleton_data.light_framebuffer_image_format;
	config.clear_colour_image = true;

	ASSERT_R1 (!pigeon_vulkan_make_render_pass(&singleton_data.rp_light_pass, config));



	// light blur
	memset(&config, 0, sizeof config);
	config.colour_image = singleton_data.light_framebuffer_image_format;
	ASSERT_R1 (!pigeon_vulkan_make_render_pass(&singleton_data.rp_light_blur, config));


    PigeonWGIImageFormat hdr_format = pigeon_vulkan_compact_hdr_framebuffer_available() ?
    	PIGEON_WGI_IMAGE_FORMAT_B10G11R11_UF_LINEAR : PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR;

	// bloom blur
	memset(&config, 0, sizeof config);
	config.colour_image = hdr_format;
	ASSERT_R1 (!pigeon_vulkan_make_render_pass(&singleton_data.rp_bloom_blur, config));

	// render
	memset(&config, 0, sizeof config);
	config.depth_mode = PIGEON_VULKAN_RENDER_PASS_DEPTH_READ_ONLY;
	config.depth_format = PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32;
	config.colour_image = hdr_format;
	ASSERT_R1 (!pigeon_vulkan_make_render_pass(&singleton_data.rp_render, config));

	// post
	memset(&config, 0, sizeof config);
    PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();
	config.colour_image = sc_info.format;
	config.colour_image_is_swapchain = true;
	ASSERT_R1 (!pigeon_vulkan_make_render_pass(&singleton_data.rp_post, config));

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

#define SHADER_PATH_PREFIX "build/standard_assets/shaders/"
#ifdef NDEBUG
	#define SPV_PATH_PREFIX "build/release/standard_assets/shaders/"
#else
	#define SPV_PATH_PREFIX "build/debug/standard_assets/shaders/"
#endif

static PIGEON_ERR_RET create_program_gl(PigeonOpenGLShaderProgram * shader, 
	const char * vs_path, const char * fs_path, 
	const char * prefix,
    unsigned int vertex_attrib_count, const char* vertex_attrib_names[])
{
	if(prefix && !*prefix) prefix = NULL;

	PigeonOpenGLShader vs = { 0 }, fs = { 0 };
	if (pigeon_opengl_load_shader(&vs, vs_path, SHADER_PATH_PREFIX, prefix, PIGEON_OPENGL_SHADER_TYPE_VERTEX)) return 1;
	if (pigeon_opengl_load_shader(&fs, fs_path, SHADER_PATH_PREFIX, prefix, PIGEON_OPENGL_SHADER_TYPE_FRAGMENT)) {
		pigeon_opengl_destroy_shader(&vs);
		return 1;
	}


	int err = pigeon_opengl_create_shader_program(shader, &vs, &fs, vertex_attrib_count, vertex_attrib_names);
	pigeon_opengl_destroy_shader(&vs);
	pigeon_opengl_destroy_shader(&fs);
	if (err) {
		return 1;
	}

	return 0;
}

static PIGEON_ERR_RET create_standard_pipeline_objects_gl(void)
{
	if (create_program_gl(&singleton_data.gl.shader_blur,
		"fullscreen.vert", "gaussian_rgb.frag",
		NULL, 0, NULL)) ASSERT_R1(false);

	pigeon_opengl_set_shader_texture_binding_index(&singleton_data.gl.shader_blur, "src_image", 0);

	const char * gaussian_light_frag_path = NULL;
	char gaussian_prefix[] = "#define COLOUR_TYPE_RGBA";

	if(singleton_data.light_image_components == 1) {
		gaussian_light_frag_path = "gaussian_light.frag.1";
		gaussian_prefix[21] = 0;
	}
	else if(singleton_data.light_image_components == 2) {
		gaussian_light_frag_path = "gaussian_light.frag";
		gaussian_prefix[0] = 0;
	}
	else if(singleton_data.light_image_components == 3) {
		gaussian_light_frag_path = "gaussian_light.frag.3";
		gaussian_prefix[23] = 0;
	}
	else if(singleton_data.light_image_components == 4) {
		gaussian_light_frag_path = "gaussian_light.frag.4";
	}


	if (gaussian_light_frag_path && 
		create_program_gl(&singleton_data.gl.shader_light_blur, "fullscreen.vert", 
		gaussian_light_frag_path, gaussian_prefix, 0, NULL)) ASSERT_R1(false);

	singleton_data.gl.shader_blur_ONE_PIXEL = 
		pigeon_opengl_get_uniform_location(&singleton_data.gl.shader_blur, "ONE_PIXEL");

	pigeon_opengl_set_shader_texture_binding_index(&singleton_data.gl.shader_light_blur, "src_image", 0);
	pigeon_opengl_set_shader_texture_binding_index(&singleton_data.gl.shader_light_blur, "depth_buffer", 1);

	singleton_data.gl.shader_light_blur_u_dist_and_half = 
		pigeon_opengl_get_uniform_location(&singleton_data.gl.shader_light_blur, "u_dist_and_half");
	singleton_data.gl.shader_light_blur_u_near_far = 
		pigeon_opengl_get_uniform_location(&singleton_data.gl.shader_light_blur, "u_near_far");
	

	if (create_program_gl(&singleton_data.gl.shader_bloom_downsample,
		"fullscreen.vert", "downsample.frag",
		"#define SC_BLOOM_DOWNSAMPLE_SAMPLES 4", 0, NULL)) return 1;

	pigeon_opengl_set_shader_texture_binding_index(&singleton_data.gl.shader_bloom_downsample, "src_image", 0);
	
	singleton_data.gl.shader_bloom_downsample_u_offset_and_min = 
		pigeon_opengl_get_uniform_location(&singleton_data.gl.shader_bloom_downsample, "u_offset_and_min");

	if (create_program_gl(&singleton_data.gl.shader_post, "post.vert", "post.frag",
		singleton_data.render_cfg.bloom ? 
			"#define SC_BLOOM true" : "#define SC_BLOOM false", 0, NULL)) return 1;
			
	pigeon_opengl_set_shader_texture_binding_index(&singleton_data.gl.shader_post, "hdr_render", 0);
	pigeon_opengl_set_shader_texture_binding_index(&singleton_data.gl.shader_post, "bloom_texture", 1);
	
	singleton_data.gl.shader_post_u_one_pixel_and_bloom_intensity = 
		pigeon_opengl_get_uniform_location(&singleton_data.gl.shader_post, "u_one_pixel_and_bloom_intensity");	


	return 0;
}

#undef SHADER_PATH

static PIGEON_ERR_RET create_pipeine(PigeonVulkanPipeline * pipeline, const char * vs_path, const char * fs_path, 
	PigeonVulkanRenderPass* render_pass, PigeonVulkanDescriptorLayout * descriptor_layout,
	unsigned int push_constant_size, unsigned int specialisation_constants, uint32_t * sc_data)
{
	if (!pipeline || !vs_path || !fs_path || !render_pass | !descriptor_layout || push_constant_size > 128) {
		ASSERT_R1(false);
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

PIGEON_ERR_RET pigeon_wgi_create_standard_pipeline_objects(void)
{
	if(OPENGL) return create_standard_pipeline_objects_gl();

#define SHADER_PATH(x) (SPV_PATH_PREFIX x ".spv")


	if (create_pipeine(&singleton_data.pipeline_blur, SHADER_PATH("fullscreen.vert"), SHADER_PATH("gaussian_rgb.frag"),
		&singleton_data.rp_bloom_blur, &singleton_data.one_texture_descriptor_layout, 16, 0, NULL)) ASSERT_R1(false);

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
	else if(singleton_data.light_image_components == 4) {
		gaussian_light_frag_path = SHADER_PATH("gaussian_light.frag.4");
	}

	if (gaussian_light_frag_path && 
		create_pipeine(&singleton_data.pipeline_light_blur, SHADER_PATH("fullscreen.vert"), gaussian_light_frag_path,
		&singleton_data.rp_light_blur, &singleton_data.two_texture_descriptor_layout, 24, 0, NULL)) ASSERT_R1(false);

	

	uint32_t sc_bloom_samples = 4;

	if (create_pipeine(&singleton_data.pipeline_bloom_downsample,
		SHADER_PATH("fullscreen.vert"), SHADER_PATH("downsample.frag"),
		&singleton_data.rp_bloom_blur, &singleton_data.one_texture_descriptor_layout, 12, 1, &sc_bloom_samples)) return 1;
	
		
	uint32_t sc_use_bloom = singleton_data.render_cfg.bloom ? 1 : 0;

	if (create_pipeine(&singleton_data.pipeline_post, SHADER_PATH("post.vert"), SHADER_PATH("post.frag"),
		&singleton_data.rp_post, &singleton_data.post_descriptor_layout, 20, 1, &sc_use_bloom)) return 1;

#undef SHADER_PATH
	return 0;
}

void pigeon_wgi_destroy_standard_pipeline_objects()
{
	if(VULKAN) {
		pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_blur);
		pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_bloom_downsample);
		pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_light_blur);
		pigeon_vulkan_destroy_pipeline(&singleton_data.pipeline_post);
	}
	else {
		pigeon_opengl_destroy_shader_program(&singleton_data.gl.shader_blur);
		pigeon_opengl_destroy_shader_program(&singleton_data.gl.shader_bloom_downsample);
		pigeon_opengl_destroy_shader_program(&singleton_data.gl.shader_light_blur);
		pigeon_opengl_destroy_shader_program(&singleton_data.gl.shader_post);
	}
}

PIGEON_ERR_RET pigeon_wgi_create_shader(PigeonWGIShader* shader, 
	const void* spv, uint32_t size_bytes, PigeonWGIShaderType type)
{
	(void) type;
	ASSERT_R1(shader && spv && size_bytes && VULKAN);
	ASSERT_R1(size_bytes % 4 == 0);

	shader->shader = malloc(sizeof *shader->shader);
	ASSERT_R1(shader->shader);

	if(pigeon_vulkan_create_shader(shader->shader, spv, size_bytes)) {
		free(shader->shader);
		return 1;
	}

	return 0;
}

bool pigeon_wgi_accepts_spirv(void)
{
	return VULKAN;
}

PIGEON_ERR_RET pigeon_wgi_create_shader2(PigeonWGIShader* shader, const char * file_name, 
	PigeonWGIShaderType type, const PigeonWGIPipelineConfig* config, PigeonWGIRenderStage stage)
{
	ASSERT_R1(shader && file_name && OPENGL);

	shader->opengl_shader = malloc(sizeof *shader->opengl_shader);
	ASSERT_R1(shader->opengl_shader);

	char prefix[256];

	if(config) {
		bool skinned = false;
		for(unsigned int i = 0; i < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES; i++) {
			if(config->vertex_attributes[i] == PIGEON_WGI_VERTEX_ATTRIBUTE_BONE) {
				skinned = true;
			}
		}

		const char * b_true = "true";
		const char * b_false = "false";

		const char * stage_sym = "OBJECT";
		if(stage == PIGEON_WGI_RENDER_STAGE_LIGHT) {
			stage_sym = "OBJECT_LIGHT";
		}
		else if (stage == PIGEON_WGI_RENDER_STAGE_DEPTH) {
			if(config->blend_function) {
				stage_sym = "OBJECT_DEPTH_ALPHA";
			}
			else {
				stage_sym = "OBJECT_DEPTH";	
			}
		}

		snprintf(prefix, sizeof prefix, 
			"#define SC_SSAO_SAMPLES %u\n"
			"#define SC_SHADOW_SAMPLES 2\n"
			"#define SC_FETCH_ALPHA %s\n"
			"#define SC_COLOUR_COMPONENTS %u\n"
			"#define SC_SSAO_ENABLED %s\n"
			"#define SC_TRANSPARENT %s\n"
			"#define %s\n"
			"%s",
			singleton_data.render_cfg.ssao ? 4 : 0,
			config->blend_function == PIGEON_WGI_BLEND_NONE ? b_false : b_true,
			singleton_data.light_image_components,
			singleton_data.render_cfg.ssao ? b_true : b_false,
			config->blend_function == PIGEON_WGI_BLEND_NONE ? b_false : b_true,
			stage_sym,
			skinned ? "#define SKINNED" : ""
		);
	}

	
	if(pigeon_opengl_load_shader(shader->opengl_shader, file_name, SHADER_PATH_PREFIX, config ? prefix : NULL,
		type == PIGEON_WGI_SHADER_TYPE_FRAGMENT ? PIGEON_OPENGL_SHADER_TYPE_FRAGMENT
			: PIGEON_OPENGL_SHADER_TYPE_VERTEX))
	{
		free(shader->shader);
		shader->shader = NULL;
		return 1;
	}
	return 0;
}

void pigeon_wgi_destroy_shader(PigeonWGIShader* shader)
{
	if(shader && shader->shader) {
		if(VULKAN) pigeon_vulkan_destroy_shader(shader->shader);
		else pigeon_opengl_destroy_shader(shader->opengl_shader);
		free(shader->shader);
		shader->shader = NULL;
	}
	else {
		assert(false);
	}
}


PIGEON_ERR_RET pigeon_wgi_create_skybox_pipeline(PigeonWGIPipeline* pipeline, PigeonWGIShader* vs, PigeonWGIShader* fs)
{
	ASSERT_R1(pipeline && vs && fs);

	PigeonWGIPipelineConfig config = {0};
	config.depth_test = true;
	config.depth_cmp_equal = true;


	if(VULKAN) {
		pipeline->pipeline = calloc(1, sizeof *pipeline->pipeline);
		ASSERT_R1(pipeline->pipeline);


		if(pigeon_vulkan_create_pipeline(pipeline->pipeline, vs->shader, fs->shader,
			0, &singleton_data.rp_render, &singleton_data.render_descriptor_layout, &config, 0, NULL)) {
			free(pipeline->pipeline);
			return 1;
		}
	}
	else {
		pipeline->gl.program = calloc(1, sizeof *pipeline->gl.program);
		ASSERT_R1(pipeline->gl.program);
		pipeline->gl.config = config;


		if(pigeon_opengl_create_shader_program(pipeline->gl.program, 
			vs->opengl_shader, fs->opengl_shader, 0, NULL))
		{
			free(pipeline->gl.program);
			return 1;
		}

		pigeon_opengl_set_uniform_buffer_binding(pipeline->gl.program, "UniformBufferObject", 0);
	}


	return 0;
}

static PIGEON_ERR_RET pigeon_wgi_create_pipeline_gl(PigeonWGIPipeline* pipeline, 
	PigeonWGIShader* vs_depth, 
	PigeonWGIShader* vs_light, PigeonWGIShader* vs, 
	PigeonWGIShader* fs_depth, // NULL for opaque objects
	PigeonWGIShader* fs_light, PigeonWGIShader* fs,
	const PigeonWGIPipelineConfig* config, bool skinned, bool transparent)
{
	pipeline->gl.program_depth = calloc(1, sizeof *pipeline->gl.program_depth);
	pipeline->gl.program_light = calloc(1, sizeof *pipeline->gl.program_light);
	pipeline->gl.program = calloc(1, sizeof *pipeline->gl.program);

#define CLEANUP() pigeon_wgi_destroy_pipeline(pipeline);

	ASSERT_R1(pipeline->gl.program_depth && pipeline->gl.program_light && pipeline->gl.program);


	const char * with_bones[5] = {
		"in_position",
		"in_bone",
		"in_uv",
		"in_normal",
		"in_tangent",
	};

	const char * without_bones[4] = {
		"in_position",
		"in_uv",
		"in_normal",
		"in_tangent",
	};

	ASSERT_R1(!pigeon_opengl_create_shader_program(pipeline->gl.program, vs->opengl_shader, fs->opengl_shader,
		skinned ? 5 : 4, skinned ? with_bones : without_bones));


	ASSERT_R1(!pigeon_opengl_create_shader_program(pipeline->gl.program_light, 
		vs_light->opengl_shader, fs_light->opengl_shader,
		skinned ? 4 : 3, skinned ? with_bones : without_bones));


	ASSERT_R1(!pigeon_opengl_create_shader_program(pipeline->gl.program_depth, 
		vs_depth->opengl_shader, fs_depth ? fs_depth->opengl_shader : NULL,
		(skinned ? 2 : 1) + (transparent ? 1 : 0),
		skinned ? with_bones : without_bones));

	PigeonOpenGLShaderProgram* programs[3] = {
		pipeline->gl.program_depth,
		pipeline->gl.program,
		pipeline->gl.program_light
	};

	for(unsigned int i = 0; i < 3; i++) {		
		pigeon_opengl_set_uniform_buffer_binding(programs[i], "UniformBufferObject", 0);
		pigeon_opengl_set_uniform_buffer_binding(programs[i], "DrawObjectUniform", 1);

		if(transparent || i==1)
			pigeon_opengl_set_shader_texture_binding_index(programs[i], "diffuse_texture", 4);

		if(skinned)
			pigeon_opengl_set_uniform_buffer_binding(programs[i], "BonesUniform", 2);
				
		if(i > 0) {
			if(i == 2 || transparent) {	
				pigeon_opengl_set_shader_texture_binding_index(programs[i], "shadow_map0", 0);
				pigeon_opengl_set_shader_texture_binding_index(programs[i], "shadow_map1", 1);
				pigeon_opengl_set_shader_texture_binding_index(programs[i], "shadow_map2", 2);
				pigeon_opengl_set_shader_texture_binding_index(programs[i], "shadow_map3", 3);
			}
		}
	}

	pigeon_opengl_set_shader_texture_binding_index(pipeline->gl.program_light, "depth_image", 5);

	if(!transparent)
		pigeon_opengl_set_shader_texture_binding_index(pipeline->gl.program, "shadow_texture", 5);

	pigeon_opengl_set_shader_texture_binding_index(pipeline->gl.program, "nmap_texture", 6);


	pipeline->gl.program_depth_mvp = -1;
	pipeline->gl.program_light_mvp = -1;
	pipeline->gl.program_mvp = -1;

	pipeline->gl.program_depth_mvp_loc = pigeon_opengl_get_uniform_location(
		pipeline->gl.program_depth, "MODEL_VIEW_PROJ_INDEX");
	pipeline->gl.program_light_mvp_loc = pigeon_opengl_get_uniform_location(
		pipeline->gl.program_light, "MODEL_VIEW_PROJ_INDEX");
	pipeline->gl.program_mvp_loc = pigeon_opengl_get_uniform_location(
		pipeline->gl.program, "MODEL_VIEW_PROJ_INDEX");

	pipeline->gl.config_depth = *config;
	pipeline->gl.config_light = *config;
	pipeline->gl.config = *config;

	
	pipeline->gl.config.depth_write = false;
	pipeline->gl.config.depth_test = true;
	pipeline->gl.config.depth_cmp_equal = true;
	
	pipeline->gl.config_light.depth_write = false;
	pipeline->gl.config_light.depth_test = true;
	pipeline->gl.config_light.depth_cmp_equal = true;
	pipeline->gl.config_light.depth_cmp_equal = true;


	pipeline->gl.config_depth.depth_write = true;
	pipeline->gl.config_depth.depth_test = true;
	pipeline->gl.config_depth.depth_only = true;
	pipeline->gl.config_depth.depth_cmp_equal = false;

	pipeline->gl.config_shadow.depth_write = true;
	pipeline->gl.config_shadow.depth_test = true;
	pipeline->gl.config_shadow.depth_only = true;
	pipeline->gl.config_shadow.depth_cmp_equal = false;
	pipeline->gl.config_shadow.depth_bias = false;

#undef CLEANUP
	return 0;
}

PIGEON_ERR_RET pigeon_wgi_create_pipeline(PigeonWGIPipeline* pipeline, 
	PigeonWGIShader* vs_depth, 
	PigeonWGIShader* vs_light, PigeonWGIShader* vs, 
	PigeonWGIShader* fs_depth, // NULL for opaque objects
	PigeonWGIShader* fs_light, PigeonWGIShader* fs,
	const PigeonWGIPipelineConfig* config, bool skinned, bool transparent)
{
	ASSERT_R1(pipeline && vs_depth && vs_light && vs && fs && fs_light && config);

	pipeline->skinned = skinned;
	pipeline->transparent = transparent;

	if(OPENGL) {
		return pigeon_wgi_create_pipeline_gl(pipeline, 
			vs_depth, vs_light, vs, 
			fs_depth, fs_light, fs, config, skinned, transparent);
	}



	pipeline->pipeline_depth = calloc(1, sizeof *pipeline->pipeline_depth);
	pipeline->pipeline_shadow_map = calloc(1, sizeof *pipeline->pipeline_shadow_map);
	pipeline->pipeline_light = calloc(1, sizeof *pipeline->pipeline_light);
	pipeline->pipeline = calloc(1, sizeof *pipeline->pipeline);

	if(!pipeline->pipeline_depth || !pipeline->pipeline_shadow_map || !pipeline->pipeline_light || !pipeline->pipeline) {
		if(pipeline->pipeline_depth) free(pipeline->pipeline_depth);
		if(pipeline->pipeline_shadow_map) free(pipeline->pipeline_shadow_map);
		if(pipeline->pipeline_light) free(pipeline->pipeline_light);
		if(pipeline->pipeline) free(pipeline->pipeline);
		ASSERT_R1(false);
	}

	#define ERR() \
		free(pipeline->pipeline_depth); \
		free(pipeline->pipeline_shadow_map); \
		free(pipeline->pipeline_light); \
		free(pipeline->pipeline); \
		ASSERT_R1(false);


	PigeonWGIPipelineConfig config2 = *config;
	config2.depth_write = false;
	config2.depth_test = true;
	config2.depth_cmp_equal = true;

	uint32_t sc_render[2] = {singleton_data.render_cfg.ssao ? 1 : 0, 
		config->blend_function == PIGEON_WGI_BLEND_NONE ? 0 : 1};
	if(pigeon_vulkan_create_pipeline(pipeline->pipeline, vs->shader, fs->shader,
		8, &singleton_data.rp_render, &singleton_data.render_descriptor_layout, &config2, 2, sc_render))
	{
		free(pipeline->pipeline);
		ERR();
	}

	config2.depth_cmp_equal = true;
	uint32_t sc[4] = {singleton_data.render_cfg.ssao ? 4 : 0, 2,
		config->blend_function == PIGEON_WGI_BLEND_NONE ? 0 : 1, singleton_data.light_image_components};
	if(pigeon_vulkan_create_pipeline(pipeline->pipeline_light, vs_light->shader, fs_light->shader,
		8, &singleton_data.rp_light_pass, &singleton_data.render_descriptor_layout, &config2, 4, sc))
	{
		pigeon_vulkan_destroy_pipeline(pipeline->pipeline);
		ERR();
	}

	config2.depth_write = true;
	config2.depth_test = true;
	config2.depth_only = true;
	config2.depth_cmp_equal = false;

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
	assert(pipeline);

	if(!pipeline) return;

	if(VULKAN) {
		if(pipeline->pipeline) {
			pigeon_vulkan_destroy_pipeline(pipeline->pipeline);
			free(pipeline->pipeline);
			pipeline->pipeline = NULL;
		}
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

	if(OPENGL) {
		if(pipeline->gl.program) {
			pigeon_opengl_destroy_shader_program(pipeline->gl.program);
			free(pipeline->gl.program);
			pipeline->gl.program = NULL;
		}
		if(pipeline->gl.program_depth) {
			pigeon_opengl_destroy_shader_program(pipeline->gl.program_depth);
			free(pipeline->gl.program_depth);
			pipeline->gl.program_depth = NULL;
		}
		if(pipeline->gl.program_light) {
			pigeon_opengl_destroy_shader_program(pipeline->gl.program_light);
			free(pipeline->gl.program_light);
			pipeline->gl.program_light = NULL;
		}
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
		case PIGEON_WGI_VERTEX_ATTRIBUTE_BONE:
			return 4;
	}
	return 0;
}