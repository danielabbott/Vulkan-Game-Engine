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
#include <pigeon/wgi/window.h>

typedef struct FramebufferImageObjects {
    PigeonVulkanImage image;
    PigeonVulkanImageView image_view;
    PigeonVulkanMemoryAllocation memory;
} FramebufferImageObjects;


typedef struct PigeonWGICommandBuffer {
    PigeonVulkanCommandPool command_pool;
	bool no_render;
	bool has_been_recorded;

	// Below fields are unused for upload command buffer

    PigeonVulkanMemoryAllocation* uniform_buffer_memory;
    PigeonVulkanBuffer* uniform_buffer;
	bool depth_only;
} PigeonWGICommandBuffer;


typedef struct PerFrameData {
    PigeonVulkanCommandPool primary_command_pool;

	PigeonWGICommandBuffer upload_command_buffer;
    PigeonWGICommandBuffer depth_command_buffer;
    PigeonWGICommandBuffer render_command_buffer;

    PigeonVulkanMemoryAllocation uniform_buffer_memory;
    PigeonVulkanBuffer uniform_buffer;
    
    // PigeonVulkanMemoryAllocation shadow_uniform_buffer_memory[MAX_SHADOW_CASTERS];
    // PigeonVulkanBuffer shadow_uniform_buffer[MAX_SHADOW_CASTERS];

    // These are per-frame because the uniform buffer is per-frame
	PigeonVulkanDescriptorPool depth_descriptor_pool;
	PigeonVulkanDescriptorPool render_descriptor_pool;

    // PigeonVulkanQueryPool query_pool; // Timers

    bool commands_in_progress;
    PigeonVulkanFence pre_render_done_fence;

	PigeonVulkanSemaphore pre_processing_done_semaphore;
	PigeonVulkanSemaphore render_done_semaphore;
	PigeonVulkanSemaphore post_processing_done_semaphore;

	PigeonVulkanSemaphore render_done_semaphore2;
	PigeonVulkanSemaphore post_processing_done_semaphore2;
} PerFrameData;

typedef struct SingletonData
{
	PigeonVulkanDescriptorLayout render_descriptor_layout;
	PigeonVulkanDescriptorLayout depth_descriptor_layout;
	PigeonVulkanDescriptorLayout shadow_map_descriptor_layout;
	PigeonVulkanDescriptorLayout one_texture_descriptor_layout;
	PigeonVulkanDescriptorLayout post_descriptor_layout;

	PigeonVulkanSampler nearest_filter_sampler;
	PigeonVulkanSampler bilinear_sampler;
	PigeonVulkanSampler shadow_sampler;
	PigeonVulkanSampler texture_sampler;

	PigeonVulkanRenderPass rp_depth;
	PigeonVulkanRenderPass rp_ssao;
	PigeonVulkanRenderPass rp_ssao_blur;
	PigeonVulkanRenderPass rp_render;
	PigeonVulkanRenderPass rp_bloom_gaussian;
	PigeonVulkanRenderPass rp_post;

	PigeonVulkanPipeline pipeline_ssao;
	PigeonVulkanPipeline pipeline_ssao_blur;
	PigeonVulkanPipeline pipeline_downsample;
	PigeonVulkanPipeline pipeline_bloom_gaussian;
	PigeonVulkanPipeline pipeline_post;

	PigeonVulkanMemoryAllocation default_textures_memory;
	PigeonVulkanImage default_1px_white_texture_image;
	PigeonVulkanImageView default_1px_white_texture_image_view;
	PigeonVulkanImageView default_1px_white_texture_array_image_view;

    FramebufferImageObjects depth_image;
    FramebufferImageObjects ssao_image;
    FramebufferImageObjects ssao_blur_image;
    FramebufferImageObjects render_image;
    FramebufferImageObjects bloom_image;
    FramebufferImageObjects bloom_gaussian_intermediate_image;

    PigeonVulkanFramebuffer depth_framebuffer;
    PigeonVulkanFramebuffer ssao_framebuffer;
    PigeonVulkanFramebuffer ssao_blur_framebuffer;
    PigeonVulkanFramebuffer render_framebuffer;
    PigeonVulkanFramebuffer bloom_framebuffer;
    PigeonVulkanFramebuffer bloom_gaussian_intermediate_framebuffer;

	PigeonVulkanDescriptorPool ssao_descriptor_pool;
	PigeonVulkanDescriptorPool ssao_blur_descriptor_pool;
	PigeonVulkanDescriptorPool bloom_downsample_descriptor_pool;
	PigeonVulkanDescriptorPool bloom_gaussian_descriptor_pool;
	PigeonVulkanDescriptorPool bloom_intermediate_gaussian_descriptor_pool;
	PigeonVulkanDescriptorPool post_process_descriptor_pool;

	// TODO this might end up being a semaphore
	PigeonVulkanFence swapchain_acquire_fence;


	// Equals number of swapchain images
	unsigned int frame_objects_count;

	// Index is frame number % frame_objects_count
	PerFrameData * per_frame_objects;

	// Index is whatever the swapchain gives us
	PigeonVulkanFramebuffer * post_framebuffers;

	unsigned int max_draw_calls;
	unsigned int max_multidraw_draw_calls;

	unsigned int multidraw_draw_index; // reset to 0 each frame

	unsigned int swapchain_image_index;
	unsigned int previous_frame_index_mod;
	unsigned int current_frame_index_mod;

} SingletonData;

#ifndef WGI_C_
extern
#endif
SingletonData pigeon_wgi_singleton_data;

#define singleton_data pigeon_wgi_singleton_data



ERROR_RETURN_TYPE pigeon_create_window(PigeonWindowParameters);

ERROR_RETURN_TYPE pigeon_wgi_create_descriptor_layouts(void);
void pigeon_wgi_destroy_descriptor_layouts(void);

ERROR_RETURN_TYPE pigeon_wgi_create_samplers(void);
void pigeon_wgi_destroy_samplers(void);


ERROR_RETURN_TYPE pigeon_wgi_create_default_textures(void);
void pigeon_wgi_destroy_default_textures(void);

ERROR_RETURN_TYPE pigeon_wgi_create_framebuffers(void);
void pigeon_wgi_destroy_framebuffers(void);

ERROR_RETURN_TYPE pigeon_wgi_create_render_passes(void);
void pigeon_wgi_destroy_render_passes(void);

ERROR_RETURN_TYPE pigeon_wgi_create_standard_pipeline_objects(void);
void pigeon_wgi_destroy_standard_pipeline_objects(void);

ERROR_RETURN_TYPE pigeon_wgi_create_per_frame_objects(void);
void pigeon_wgi_destroy_per_frame_objects(void);

ERROR_RETURN_TYPE pigeon_wgi_create_sync_objects(void);
void pigeon_wgi_destroy_sync_objects(void);

void pigeon_wgi_destroy_descriptor_pools(void);
void pigeon_wgi_set_global_descriptors(void);
ERROR_RETURN_TYPE pigeon_wgi_create_descriptor_pools(void);
