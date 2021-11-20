#pragma once

#include "image.h"
#include "renderpass.h"

struct VkFramebuffer_T;

typedef struct PigeonVulkanFramebuffer {
	struct VkFramebuffer_T* vk_framebuffer;
	unsigned int width, height;
} PigeonVulkanFramebuffer;

PIGEON_ERR_RET pigeon_vulkan_create_framebuffer(
	PigeonVulkanFramebuffer*, PigeonVulkanImageView* depth, PigeonVulkanImageView* colour, PigeonVulkanRenderPass*);

PIGEON_ERR_RET pigeon_vulkan_create_framebuffer2(PigeonVulkanFramebuffer*, PigeonVulkanImageView* depth,
	PigeonVulkanImageView* colour, PigeonVulkanImageView* colour2, PigeonVulkanRenderPass*);

void pigeon_vulkan_destroy_framebuffer(PigeonVulkanFramebuffer*);
