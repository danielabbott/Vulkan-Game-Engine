#define WGI_C_

#include "singleton.h"

#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/wgi/vulkan/vulkan.h>
#include <pigeon/wgi/wgi.h>
#include <pigeon/wgi/pipeline.h>
#include <pigeon/wgi/textures.h>
#include <string.h>
#include <cglm/affine.h>
#include <pigeon/util.h>


ERROR_RETURN_TYPE pigeon_wgi_init(PigeonWindowParameters window_parameters, bool prefer_dedicated_gpu)
{
	pigeon_wgi_deinit();


	ASSERT_1(!pigeon_create_window(window_parameters));
	ASSERT_1(!pigeon_create_vulkan_context(prefer_dedicated_gpu));

	if (pigeon_vulkan_create_swapchain()) return 1;

	if (pigeon_wgi_create_render_passes()) return 1;
	if (pigeon_wgi_create_framebuffers()) return 1;
	if (pigeon_wgi_create_descriptor_layouts()) return 1;
	if (pigeon_wgi_create_samplers()) return 1;
	if (pigeon_wgi_create_descriptor_pools()) return 1;
	pigeon_wgi_set_global_descriptors();

	/* SSAO shader, post-processing shader, etc. */
	if (pigeon_wgi_create_standard_pipeline_objects()) return 1;

	/* Default textures are 1px white, 1px black, 2x2px missing texture */
	if (pigeon_wgi_create_default_textures()) return 1;

	ASSERT_1(!pigeon_wgi_create_sync_objects());

	if (pigeon_wgi_create_per_frame_objects()) return 1;





	return 0;
}

void pigeon_wgi_wait_idle(void)
{
	pigeon_vulkan_wait_idle();
}

void pigeon_wgi_deinit(void)
{
	pigeon_vulkan_wait_idle();

	pigeon_wgi_destroy_per_frame_objects();
	pigeon_wgi_destroy_sync_objects();
	pigeon_wgi_destroy_default_textures();
	pigeon_wgi_destroy_standard_pipeline_objects();
	pigeon_vulkan_destroy_swapchain();
	pigeon_wgi_destroy_descriptor_pools();
	pigeon_wgi_destroy_samplers();
	pigeon_wgi_destroy_descriptor_layouts();
	pigeon_wgi_destroy_framebuffers();
	pigeon_wgi_destroy_render_passes();
	pigeon_destroy_vulkan_context();
	pigeon_wgi_destroy_window();
	memset(&singleton_data, 0, sizeof singleton_data);
}

// https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/
void pigeon_wgi_perspective(mat4 m, float fovy, float aspect, float nearZ)
{
    float f = 1.0f / tanf(glm_rad(fovy) * 0.5f);

	glm_mat4_zero(m);

	// x
	m[0][0] = f / aspect;

	// y
	m[1][1] = -f;

	// z
	m[3][2] = nearZ;

	// w
	m[2][3] = -1;
}

void pigeon_wgi_get_normal_model_matrix(const mat4 model, mat4 normal_model_matrix)
{
	glm_mat4_inv((vec4*)model, normal_model_matrix);
	glm_mat4_transpose(normal_model_matrix);
}


bool pigeon_wgi_bc1_optimal_available(void)
{
	return pigeon_vulkan_bc1_optimal_available();
}
bool pigeon_wgi_bc3_optimal_available(void)
{
	return pigeon_vulkan_bc3_optimal_available();
}
bool pigeon_wgi_bc5_optimal_available(void)
{
	return pigeon_vulkan_bc5_optimal_available();
}
bool pigeon_wgi_bc7_optimal_available(void)
{
	return pigeon_vulkan_bc7_optimal_available();
}
bool pigeon_wgi_etc1_optimal_available(void)
{
	return pigeon_vulkan_etc1_optimal_available();
}
bool pigeon_wgi_etc2_optimal_available(void)
{
	return pigeon_vulkan_etc2_optimal_available();
}
bool pigeon_wgi_etc2_rgba_optimal_available(void)
{
	return pigeon_vulkan_etc2_rgba_optimal_available();
}
