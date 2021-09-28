#define WGI_C_

#include "singleton.h"

#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/wgi/vulkan/vulkan.h>
#include <pigeon/wgi/wgi.h>
#include <pigeon/wgi/pipeline.h>
#include <pigeon/wgi/textures.h>
#include <string.h>
#include <cglm/affine.h>
#include <cglm/clipspace/persp_rh_zo.h>
#include <pigeon/util.h>
#include <pigeon/wgi/rendergraph.h>

static ERROR_RETURN_TYPE set_render_graph(PigeonWGIRenderConfig render_cfg)
{
	singleton_data.render_graph = render_cfg;

	singleton_data.light_image_components = render_cfg.ssao ? 1 : 0;
	singleton_data.light_image_components += render_cfg.shadow_casting_lights;
	ASSERT_1(singleton_data.light_image_components >= 0 && singleton_data.light_image_components <= 4);

	if(singleton_data.light_image_components == 0) {
		singleton_data.light_framebuffer_image_format = PIGEON_WGI_IMAGE_FORMAT_NONE;
	}
	else if(singleton_data.light_image_components == 1) {
		singleton_data.light_framebuffer_image_format = PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR;
	}
	else if(singleton_data.light_image_components == 2) {
		singleton_data.light_framebuffer_image_format = PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR;
	}
	else if(singleton_data.light_image_components == 3) {
		singleton_data.light_framebuffer_image_format = PIGEON_WGI_IMAGE_FORMAT_A2B10G10R10_LINEAR;
	}
	else {
		singleton_data.light_framebuffer_image_format = PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR;
	}

	return 0;
}

ERROR_RETURN_TYPE pigeon_wgi_init(PigeonWindowParameters window_parameters, 
	bool prefer_dedicated_gpu,
	PigeonWGIRenderConfig render_cfg, float znear, float zfar)
{
	pigeon_wgi_deinit();
	ASSERT_1(!set_render_graph(render_cfg));
	

	pigeon_wgi_set_depth_range(znear, zfar);


	ASSERT_1(!pigeon_create_window(window_parameters));
	ASSERT_1(!pigeon_create_vulkan_context(prefer_dedicated_gpu));

	if (pigeon_vulkan_create_swapchain()) return 1;

	if (pigeon_wgi_create_render_passes()) return 1;
	if (pigeon_wgi_create_framebuffers()) return 1;
	if (pigeon_wgi_create_descriptor_layouts()) return 1;
	if (pigeon_wgi_create_samplers()) return 1;
	if (pigeon_wgi_create_descriptor_pools()) return 1;
	if (pigeon_wgi_create_default_textures()) return 1;

	pigeon_wgi_set_global_descriptors();

	if (pigeon_wgi_create_standard_pipeline_objects()) return 1;


	ASSERT_1(!pigeon_wgi_create_sync_objects());

	if (pigeon_wgi_create_per_frame_objects()) return 1;




	return 0;
}

void pigeon_wgi_wait_idle(void)
{
	pigeon_vulkan_wait_idle();
}


ERROR_RETURN_TYPE pigeon_wgi_recreate_swapchain(void)
{
    pigeon_wgi_wait_idle();
    pigeon_wgi_destroy_per_frame_objects();
	pigeon_wgi_destroy_framebuffers();
    pigeon_vulkan_destroy_swapchain();

    int err = pigeon_vulkan_create_swapchain();
    if(err) return err;

    ASSERT_1(!pigeon_wgi_create_framebuffers());
    pigeon_wgi_set_global_descriptors();
    ASSERT_1(!pigeon_wgi_create_per_frame_objects());
    return 0;
}

void pigeon_wgi_deinit(void)
{
	pigeon_vulkan_wait_idle();

	memset(singleton_data.shadow_parameters, 0, sizeof singleton_data.shadow_parameters);
	(void)pigeon_wgi_assign_shadow_framebuffers();

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
