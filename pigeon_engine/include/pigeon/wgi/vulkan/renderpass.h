#pragma once

#include "../image_formats.h"
#include <pigeon/util.h>
#include <stdbool.h>

struct VkRenderPass_T;

typedef enum {
	PIGEON_VULKAN_RENDER_PASS_DEPTH_NONE, // No depth buffer
	PIGEON_VULKAN_RENDER_PASS_DEPTH_READ_ONLY, // Read-only depth buffer provided for depth testing
	PIGEON_VULKAN_RENDER_PASS_DEPTH_KEEP, // Depth buffer will be written to for depth testing and data will be
										  // available after
	PIGEON_VULKAN_RENDER_PASS_DEPTH_DISCARD // Depth buffer will be written to for depth testing then discarded
} PigeonVulkanRenderPassDepthMode;

typedef struct PigeonVulkanRenderPassConfig {
	PigeonVulkanRenderPassDepthMode depth_mode;
	PigeonWGIImageFormat depth_format;

	PigeonWGIImageFormat colour_image;
	bool colour_image_is_swapchain; // used if colour_image != none
	bool clear_colour_image; // false = ignore previous contents of framebuffer
	bool leave_as_transfer_src; // If render target is going to be blitted or loaded to host memory

	PigeonWGIImageFormat colour_image2;
	// 2nd colour attatchment can not be swapchain
	bool clear_colour_image2;
	bool leave_as_transfer_src2;
} PigeonVulkanRenderPassConfig;

typedef struct PigeonVulkanRenderPass {
	struct VkRenderPass_T* vk_renderpass;
	bool has_colour_image;
	bool has_2_colour_images;
	bool has_depth_image;
	bool has_writeable_depth_image;
} PigeonVulkanRenderPass;

PIGEON_ERR_RET pigeon_vulkan_make_render_pass(PigeonVulkanRenderPass*, PigeonVulkanRenderPassConfig);
void pigeon_vulkan_destroy_render_pass(PigeonVulkanRenderPass*);
