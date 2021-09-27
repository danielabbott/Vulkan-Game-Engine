#include <pigeon/wgi/vulkan/framebuffer.h>
#include "singleton.h"
#include <pigeon/util.h>

ERROR_RETURN_TYPE pigeon_vulkan_create_framebuffer2(PigeonVulkanFramebuffer* framebuffer, 
    PigeonVulkanImageView * depth, PigeonVulkanImageView * colour, PigeonVulkanImageView * colour2,
    PigeonVulkanRenderPass* render_pass)
{
    ASSERT_1(framebuffer && render_pass);
    ASSERT_1(depth || colour);
    if(colour2) ASSERT_1(colour);

    VkFramebufferCreateInfo framebuffer_create = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebuffer_create.renderPass = render_pass->vk_renderpass;
    
    unsigned int number_of_attachments = 0;
    VkImageView attachments[3];

    if(colour) {
        attachments[number_of_attachments++] = colour->vk_image_view;

        framebuffer_create.width = colour->width;
        framebuffer_create.height = colour->height;
		framebuffer_create.layers = colour->layers;
    }
    if(colour2) {
        attachments[number_of_attachments++] = colour2->vk_image_view;

        if(colour) {
            assert(framebuffer_create.width == colour2->width &&
            framebuffer_create.height == colour2->height &&
            framebuffer_create.layers == colour2->layers);
        }
    }
    if(depth) {
        attachments[number_of_attachments++] = depth->vk_image_view;

        if(colour) {
            assert(framebuffer_create.width == depth->width &&
            framebuffer_create.height == depth->height &&
            framebuffer_create.layers == depth->layers);
        }

        framebuffer_create.width = depth->width;
        framebuffer_create.height = depth->height;
		framebuffer_create.layers = depth->layers;
    }

    framebuffer_create.pAttachments = attachments;
	framebuffer_create.attachmentCount = number_of_attachments;

    ASSERT__1(vkCreateFramebuffer(vkdev, &framebuffer_create, NULL, &framebuffer->vk_framebuffer) == VK_SUCCESS, 
					"vkCreateFramebuffer error");
    return 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_create_framebuffer(PigeonVulkanFramebuffer* framebuffer, 
    PigeonVulkanImageView * depth, PigeonVulkanImageView * colour, PigeonVulkanRenderPass* render_pass)
{
    assert(framebuffer && render_pass);
    assert(depth || colour);

    VkFramebufferCreateInfo framebuffer_create = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebuffer_create.renderPass = render_pass->vk_renderpass;
    
    unsigned int number_of_attachments = 0;
    VkImageView attachments[2];

    if(colour) {
        attachments[number_of_attachments++] = colour->vk_image_view;

        framebuffer_create.width = colour->width;
        framebuffer_create.height = colour->height;
		framebuffer_create.layers = colour->layers;
    }
    if(depth) {
        attachments[number_of_attachments++] = depth->vk_image_view;

        if(colour) {
            assert(framebuffer_create.width == depth->width &&
            framebuffer_create.height == depth->height &&
            framebuffer_create.layers == depth->layers);
        }

        framebuffer_create.width = depth->width;
        framebuffer_create.height = depth->height;
		framebuffer_create.layers = depth->layers;
    }

    framebuffer_create.pAttachments = attachments;
	framebuffer_create.attachmentCount = number_of_attachments;

    ASSERT__1(vkCreateFramebuffer(vkdev, &framebuffer_create, NULL, &framebuffer->vk_framebuffer) == VK_SUCCESS, 
					"vkCreateFramebuffer error");
    return 0;
}

void pigeon_vulkan_destroy_framebuffer(PigeonVulkanFramebuffer* framebuffer)
{
	if (framebuffer && framebuffer->vk_framebuffer) {
		vkDestroyFramebuffer(vkdev, framebuffer->vk_framebuffer, NULL);
		framebuffer->vk_framebuffer = NULL;
	}
	else {
		assert(false);
	}
}
