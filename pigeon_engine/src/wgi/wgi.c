#define WGI_C_

#include "singleton.h"

#include <cglm/affine.h>
#include <cglm/clipspace/persp_rh_zo.h>
#include <pigeon/assert.h>
#include <pigeon/wgi/opengl/limits.h>
#include <pigeon/wgi/pipeline.h>
#include <pigeon/wgi/rendergraph.h>
#include <pigeon/wgi/textures.h>
#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/wgi/vulkan/vulkan.h>
#include <pigeon/wgi/wgi.h>
#include <string.h>

void pigeon_wgi_validate_render_cfg(PigeonWGIRenderConfig* render_cfg) { (void)render_cfg; }

void pigeon_wgi_set_bloom_intensity(float i) { singleton_data.bloom_intensity = i; }

void pigeon_wgi_set_brightness(float b) { singleton_data.brightness = b; }
void pigeon_wgi_set_ambient(float r, float g, float b)
{
	singleton_data.ambient[0] = r;
	singleton_data.ambient[1] = g;
	singleton_data.ambient[2] = b;
}
void pigeon_wgi_set_ssao_cutoff(float c) { singleton_data.ssao_cutoff = c; }

PIGEON_ERR_RET pigeon_wgi_init(PigeonWindowParameters window_parameters, bool prefer_dedicated_gpu, bool prefer_opengl,
	PigeonWGIRenderConfig render_cfg, float znear, float zfar)
{
	pigeon_wgi_deinit();

	pigeon_wgi_validate_render_cfg(&render_cfg);
	singleton_data.full_render_cfg = render_cfg;
	singleton_data.active_render_cfg = render_cfg;

	singleton_data.bloom_intensity = 1;
	singleton_data.brightness = 0.75f;
	singleton_data.ambient[0] = 0.08f;
	singleton_data.ambient[1] = 0.08f;
	singleton_data.ambient[2] = 0.083f;
	singleton_data.ssao_cutoff = 0.02f;

	pigeon_wgi_set_depth_range(znear, zfar);

	if (pigeon_create_window(window_parameters, prefer_opengl)) {
		ASSERT_R1(!pigeon_create_window(window_parameters, !prefer_opengl));
		VULKAN = prefer_opengl;
		OPENGL = !prefer_opengl;
	} else {
		VULKAN = !prefer_opengl;
		OPENGL = prefer_opengl;
	}

	if (OPENGL)
		pigeon_opengl_get_limits();

	if (VULKAN) {
		if (pigeon_create_vulkan_context(prefer_dedicated_gpu))
			return 1;
		if (pigeon_vulkan_create_swapchain())
			return 1;
		if (pigeon_wgi_create_render_passes())
			return 1;
	}

	if (pigeon_wgi_bc1_optimal_available())
		puts("BC1 supported");
	if (pigeon_wgi_bc3_optimal_available())
		puts("BC3 supported");
	if (pigeon_wgi_bc5_optimal_available())
		puts("BC5 supported");
	if (pigeon_wgi_bc7_optimal_available())
		puts("BC7 supported");
	if (pigeon_wgi_etc1_optimal_available())
		puts("ETC1 supported");
	if (pigeon_wgi_etc2_optimal_available())
		puts("ETC2 RGB supported");
	if (pigeon_wgi_etc2_rgba_optimal_available())
		puts("ETC2 RGBA supported");

	if (pigeon_wgi_create_framebuffers())
		return 1;

	if (VULKAN) {
		if (pigeon_wgi_create_descriptor_layouts())
			return 1;
		if (pigeon_wgi_create_samplers())
			return 1;
		if (pigeon_wgi_create_descriptor_pools())
			return 1;
	}

	if (pigeon_wgi_create_default_textures())
		return 1;

	if (VULKAN)
		pigeon_wgi_set_global_descriptors();

	if (pigeon_wgi_create_standard_pipeline_objects())
		return 1;

	if (pigeon_wgi_create_per_frame_objects())
		return 1;

	if (OPENGL) {
		if (pigeon_opengl_create_empty_vao(&singleton_data.gl.empty_vao))
			return 1;
	}

	return 0;
}

void pigeon_wgi_wait_idle(void)
{
	if (VULKAN)
		pigeon_vulkan_wait_idle();
}

PIGEON_ERR_RET pigeon_wgi_recreate_swapchain(void)
{

	if (OPENGL) {
		PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();
		if (sc_info.width < 16 || sc_info.height < 16)
			return 2;
	}

	pigeon_wgi_wait_idle();
	pigeon_wgi_destroy_per_frame_objects();
	pigeon_wgi_destroy_framebuffers();

	if (VULKAN) {
		pigeon_vulkan_destroy_swapchain();
		int err = pigeon_vulkan_create_swapchain();
		if (err)
			return err;
	}

	ASSERT_R1(!pigeon_wgi_create_framebuffers());
	if (VULKAN) {
		pigeon_wgi_set_global_descriptors();
	}
	ASSERT_R1(!pigeon_wgi_create_per_frame_objects());
	return 0;
}

void pigeon_wgi_deinit(void)
{
	pigeon_vulkan_wait_idle();

	if (OPENGL) {
		pigeon_opengl_destroy_vao(&singleton_data.gl.empty_vao);
	}

	memset(singleton_data.shadow_parameters, 0, sizeof singleton_data.shadow_parameters);
	if (pigeon_wgi_assign_shadow_framebuffers()) { }

	pigeon_wgi_destroy_per_frame_objects();
	pigeon_wgi_destroy_default_textures();
	pigeon_wgi_destroy_standard_pipeline_objects();
	if (VULKAN) {
		pigeon_vulkan_destroy_swapchain();
		pigeon_wgi_destroy_descriptor_pools();
		pigeon_wgi_destroy_samplers();
		pigeon_wgi_destroy_descriptor_layouts();
	}
	pigeon_wgi_destroy_framebuffers();

	if (VULKAN) {
		pigeon_wgi_destroy_render_passes();
		pigeon_destroy_vulkan_context();
	}

	pigeon_wgi_destroy_window();

	memset(&singleton_data, 0, sizeof singleton_data);
}

void pigeon_wgi_set_depth_range(float znear, float zfar)
{
	singleton_data.znear = znear;
	singleton_data.zfar = zfar;
}

void pigeon_wgi_perspective(mat4 m, float fovy, float aspect)
{

	glm_perspective_rh_zo(glm_rad(fovy), aspect, singleton_data.zfar, singleton_data.znear, m);
	m[1][1] *= -1.0f;
}

void pigeon_wgi_get_normal_model_matrix(const mat4 model, mat4 normal_model_matrix)
{
	mat4 m;
	glm_mat4_inv((vec4*)model, m);

#if defined(__SSE__) || defined(__SSE2__)
	glm_mat4_transpose_to(m, normal_model_matrix);
#else
	// glm_mat4_transpose_to does not write in order (preferred for writing uniform data)
	//	so it is reimplemented here
	normal_model_matrix[0][0] = model[0][0];
	normal_model_matrix[0][1] = model[1][0];
	normal_model_matrix[0][2] = model[2][0];
	normal_model_matrix[0][3] = model[3][0];
	normal_model_matrix[1][0] = model[0][1];
	normal_model_matrix[1][1] = model[1][1];
	normal_model_matrix[1][2] = model[2][1];
	normal_model_matrix[1][3] = model[3][1];
	normal_model_matrix[2][0] = model[0][2];
	normal_model_matrix[2][1] = model[1][2];
	normal_model_matrix[2][2] = model[2][2];
	normal_model_matrix[2][3] = model[3][2];
	normal_model_matrix[3][0] = model[0][3];
	normal_model_matrix[3][1] = model[1][3];
	normal_model_matrix[3][2] = model[2][3];
	normal_model_matrix[3][3] = model[3][3];
#endif
}

bool pigeon_wgi_bc1_optimal_available(void)
{
	if (VULKAN)
		return pigeon_vulkan_bc1_optimal_available();
	return pigeon_opengl_bc1_optimal_available();
}
bool pigeon_wgi_bc3_optimal_available(void)
{
	if (VULKAN)
		return pigeon_vulkan_bc3_optimal_available();
	return pigeon_opengl_bc3_optimal_available();
}
bool pigeon_wgi_bc5_optimal_available(void)
{
	if (VULKAN)
		return pigeon_vulkan_bc5_optimal_available();
	return pigeon_opengl_bc5_optimal_available();
}
bool pigeon_wgi_bc7_optimal_available(void)
{
	if (VULKAN)
		return pigeon_vulkan_bc7_optimal_available();
	return pigeon_opengl_bc7_optimal_available();
}
bool pigeon_wgi_etc1_optimal_available(void)
{
	if (VULKAN)
		return pigeon_vulkan_etc1_optimal_available();
	return pigeon_opengl_etc1_optimal_available();
}
bool pigeon_wgi_etc2_optimal_available(void)
{
	if (VULKAN)
		return pigeon_vulkan_etc2_optimal_available();
	return pigeon_opengl_etc2_optimal_available();
}
bool pigeon_wgi_etc2_rgba_optimal_available(void)
{
	if (VULKAN)
		return pigeon_vulkan_etc2_rgba_optimal_available();
	return pigeon_opengl_etc2_rgba_optimal_available();
}

PigeonWGISwapchainInfo pigeon_wgi_get_swapchain_info(void)
{
	if (OPENGL)
		return pigeon_opengl_get_swapchain_info();
	else
		return pigeon_vulkan_get_swapchain_info();
}
