#pragma once

#include "renderpass.h"
#include "image.h"

struct VkFramebuffer_T;

typedef struct PigeonVulkanFramebuffer
{
	struct VkFramebuffer_T* vk_framebuffer;
} PigeonVulkanFramebuffer;

ERROR_RETURN_TYPE pigeon_vulkan_create_framebuffer(PigeonVulkanFramebuffer*, 
    PigeonVulkanImageView * depth, PigeonVulkanImageView * colour, PigeonVulkanRenderPass*);

void pigeon_vulkan_destroy_framebuffer(PigeonVulkanFramebuffer*);
