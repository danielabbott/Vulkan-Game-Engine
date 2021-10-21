#pragma once

#include <pigeon/wgi/vulkan/descriptor.h>
#include <pigeon/wgi/vulkan/sampler.h>
#include <pigeon/wgi/vulkan/renderpass.h>
#include <pigeon/wgi/vulkan/pipeline.h>
#include <pigeon/wgi/vulkan/image.h>
#include <pigeon/wgi/vulkan/framebuffer.h>
#include <pigeon/wgi/vulkan/fence.h>
#include <pigeon/wgi/vulkan/semaphore.h>
#include <pigeon/wgi/vulkan/command.h>
#include <pigeon/wgi/vulkan/query.h>
#include <pigeon/wgi/window.h>
#include <pigeon/wgi/shadow.h>
#include <pigeon/wgi/uniform.h>
#include <pigeon/wgi/rendergraph.h>
#include <pigeon/wgi/swapchain.h>
#include <pigeon/wgi/opengl/texture.h>
#include <pigeon/wgi/opengl/framebuffer.h>
#include <pigeon/wgi/opengl/shader.h>
#include <pigeon/wgi/opengl/buffer.h>


struct PigeonWGIArrayTexture;

typedef struct FramebufferImageObjects {
	union {
		struct {
			PigeonVulkanImage image;
			PigeonVulkanImageView image_view;
			PigeonVulkanMemoryAllocation memory;
		};
		PigeonOpenGLTexture gltex2d;
	};
} FramebufferImageObjects;


typedef struct PigeonWGICommandBuffer {
    PigeonVulkanCommandPool command_pool;
	bool no_render;
	bool has_been_recorded;

	// Below fields are unused for upload command buffer

	bool depth_only, shadow, light_pass;
	unsigned int mvp_index; // 0 for non-shadow render
} PigeonWGICommandBuffer;


typedef struct PerFrameData {
	PigeonWGICommandBuffer upload_command_buffer;
    PigeonWGICommandBuffer depth_command_buffer;
	PigeonWGICommandBuffer shadow_command_buffers[4];
    PigeonWGICommandBuffer light_pass_command_buffer;
    PigeonWGICommandBuffer render_command_buffer;

	union {
		struct {
			PigeonVulkanCommandPool primary_command_pool;

			PigeonVulkanMemoryAllocation uniform_buffer_memory;
			PigeonVulkanBuffer uniform_buffer;

			// These are per-frame because the uniform buffer is per-frame
			PigeonVulkanDescriptorPool depth_descriptor_pool;
			PigeonVulkanDescriptorPool light_pass_descriptor_pool;
			PigeonVulkanDescriptorPool render_descriptor_pool;

			bool commands_in_progress;
			PigeonVulkanFence pre_render_done_fence;

			PigeonVulkanSemaphore pre_processing_done_semaphore;
			PigeonVulkanSemaphore render_done_semaphore;
			PigeonVulkanSemaphore post_processing_done_semaphore;

			PigeonVulkanSemaphore render_done_semaphore2;
			PigeonVulkanSemaphore post_processing_done_semaphore2;
			PigeonVulkanSemaphore swapchain_acquire_semaphore;

			PigeonVulkanTimerQueryPool timer_query_pool;
		};
		struct {
			PigeonOpenGLBuffer uniform_buffer;
		} gl;
	};

    
	bool first_frame_submitted; // set to true when a frame has been rendered using this PerFrameData struct
} PerFrameData;

typedef struct SingletonData
{
	bool using_vulkan;
	bool using_opengl;

	PigeonWGIRenderConfig render_cfg;

	float bloom_intensity;

	unsigned int light_image_components;
	PigeonWGIImageFormat light_framebuffer_image_format;

	bool shadow_framebuffer_assigned[4];

	FramebufferImageObjects depth_image;
	FramebufferImageObjects shadow_images[4]; // ** images may be bigger than necessary
	FramebufferImageObjects light_image;
	FramebufferImageObjects light_blur_image;
	FramebufferImageObjects render_image;
	FramebufferImageObjects bloom1_image;
	FramebufferImageObjects bloom2_image;

	union{
		struct {
			PigeonVulkanDescriptorLayout depth_descriptor_layout;
			PigeonVulkanDescriptorLayout one_texture_descriptor_layout;
			PigeonVulkanDescriptorLayout two_texture_descriptor_layout;
			PigeonVulkanDescriptorLayout render_descriptor_layout;
			PigeonVulkanDescriptorLayout post_descriptor_layout;

			PigeonVulkanSampler nearest_filter_sampler;
			PigeonVulkanSampler bilinear_sampler;
			PigeonVulkanSampler shadow_sampler;
			PigeonVulkanSampler texture_sampler;

			PigeonVulkanRenderPass rp_depth;
			PigeonVulkanRenderPass rp_light_pass;
			PigeonVulkanRenderPass rp_light_blur;
			PigeonVulkanRenderPass rp_bloom_blur;
			PigeonVulkanRenderPass rp_render;
			PigeonVulkanRenderPass rp_post;

			PigeonVulkanPipeline pipeline_bloom_downsample;
			PigeonVulkanPipeline pipeline_light_blur;
			PigeonVulkanPipeline pipeline_blur;
			PigeonVulkanPipeline pipeline_post;

			PigeonVulkanMemoryAllocation default_textures_memory;
			PigeonVulkanMemoryAllocation default_shadow_map_memory;

			PigeonVulkanImage default_1px_white_texture_image;
			PigeonVulkanImageView default_1px_white_texture_image_view;
			PigeonVulkanImageView default_1px_white_texture_array_image_view;

			PigeonVulkanImage default_shadow_map_image;
			PigeonVulkanImageView default_shadow_map_image_view;

			PigeonVulkanFramebuffer depth_framebuffer;
			PigeonVulkanFramebuffer shadow_framebuffers[4];
			PigeonVulkanFramebuffer light_framebuffer;
			PigeonVulkanFramebuffer light_blur1_framebuffer;
			PigeonVulkanFramebuffer light_blur2_framebuffer;
			PigeonVulkanFramebuffer render_framebuffer;
			PigeonVulkanFramebuffer bloom1_framebuffer;
			PigeonVulkanFramebuffer bloom2_framebuffer;

			PigeonVulkanDescriptorPool light_blur1_descriptor_pool;
			PigeonVulkanDescriptorPool light_blur2_descriptor_pool;
			PigeonVulkanDescriptorPool bloom_downsample_descriptor_pool;
			PigeonVulkanDescriptorPool bloom1_descriptor_pool;
			PigeonVulkanDescriptorPool bloom2_descriptor_pool;
			PigeonVulkanDescriptorPool post_process_descriptor_pool;
		};
		struct {
			PigeonOpenGLTexture default_1px_white_texture_image;
			PigeonOpenGLTexture default_shadow_map_image;

			PigeonOpenGLFramebuffer depth_framebuffer;
			PigeonOpenGLFramebuffer shadow_framebuffers[4];
			PigeonOpenGLFramebuffer light_framebuffer;
			PigeonOpenGLFramebuffer light_blur1_framebuffer;
			PigeonOpenGLFramebuffer light_blur2_framebuffer;
			PigeonOpenGLFramebuffer render_framebuffer;
			PigeonOpenGLFramebuffer bloom1_framebuffer;
			PigeonOpenGLFramebuffer bloom2_framebuffer;

			PigeonOpenGLShaderProgram shader_bloom_downsample;
			PigeonOpenGLShaderProgram shader_light_blur;
			PigeonOpenGLShaderProgram shader_blur;
			PigeonOpenGLShaderProgram shader_post;

			int shader_light_blur_u_dist_and_half;
			int shader_light_blur_u_near_far;

			int shader_bloom_downsample_u_offset_and_min;

			int shader_blur_ONE_PIXEL;

			int shader_post_u_one_pixel_and_bloom_intensity;

			PigeonOpenGLVAO empty_vao;

			struct PigeonWGIArrayTexture* bound_textures[59];

			PigeonWGIPipelineConfig full_screen_tri_cfg;
		} gl;
	};

	


	// Equals number of swapchain images
	unsigned int frame_objects_count;

	// Index is frame number % frame_objects_count
	PerFrameData * per_frame_objects;

	// Index is whatever the swapchain gives us
	PigeonVulkanFramebuffer * post_framebuffers;

	unsigned int max_draws;
	unsigned int max_multidraw_draws;
	unsigned int total_bones;

	unsigned int multidraw_draw_index; // reset to 0 each frame

	unsigned int swapchain_image_index;
	unsigned int previous_frame_index_mod;
	unsigned int current_frame_index_mod;

	PigeonWGIShadowParameters shadow_parameters[4];

	float znear, zfar;

} SingletonData;

#ifndef WGI_C_
extern
#endif
SingletonData pigeon_wgi_singleton_data;

#define singleton_data pigeon_wgi_singleton_data
#define VULKAN pigeon_wgi_singleton_data.using_vulkan
#define OPENGL pigeon_wgi_singleton_data.using_opengl


PIGEON_ERR_RET pigeon_create_window(PigeonWindowParameters, bool use_opengl);
PigeonWGISwapchainInfo pigeon_opengl_get_swapchain_info(void);
void pigeon_wgi_swap_buffers(void);


PIGEON_ERR_RET pigeon_wgi_create_descriptor_layouts(void);
void pigeon_wgi_destroy_descriptor_layouts(void);

PIGEON_ERR_RET pigeon_wgi_create_samplers(void);
void pigeon_wgi_destroy_samplers(void);


PIGEON_ERR_RET pigeon_wgi_create_default_textures(void);
void pigeon_wgi_destroy_default_textures(void);

PIGEON_ERR_RET pigeon_wgi_create_framebuffers(void);
void pigeon_wgi_destroy_framebuffers(void);

PIGEON_ERR_RET pigeon_wgi_create_render_passes(void);
void pigeon_wgi_destroy_render_passes(void);

PIGEON_ERR_RET pigeon_wgi_create_standard_pipeline_objects(void);
void pigeon_wgi_destroy_standard_pipeline_objects(void);

PIGEON_ERR_RET pigeon_wgi_create_per_frame_objects(void);
void pigeon_wgi_destroy_per_frame_objects(void);

void pigeon_wgi_destroy_descriptor_pools(void);
void pigeon_wgi_set_global_descriptors(void);
PIGEON_ERR_RET pigeon_wgi_create_descriptor_pools(void);

PIGEON_ERR_RET pigeon_wgi_assign_shadow_framebuffers(void);
void pigeon_wgi_set_shadow_uniforms(PigeonWGISceneUniformData* data, PigeonWGIDrawObject *, unsigned int draw_count);

int pigeon_wgi_create_framebuffer_images(FramebufferImageObjects * objects,
    PigeonWGIImageFormat format, unsigned int width, unsigned int height,
    bool to_be_transfer_src, bool to_be_transfer_dst, bool shadow);

