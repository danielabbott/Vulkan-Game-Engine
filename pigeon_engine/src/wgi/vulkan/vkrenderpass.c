#include "singleton.h"
#include <pigeon/wgi/vulkan/renderpass.h>
#include <pigeon/util.h>



static void get_attachments(const PigeonVulkanRenderPassConfig config, VkAttachmentDescription attachments[2], 
	unsigned int* attachment_count, VkAttachmentReference* colour_attachment_ref, VkAttachmentReference* depth_attachment_ref)
{
	assert(attachments && attachment_count && colour_attachment_ref && depth_attachment_ref);

	unsigned int i = 0;

	if (config.colour_image != PIGEON_WGI_IMAGE_FORMAT_NONE) {
		attachments[0].format = pigeon_get_vulkan_image_format(config.colour_image);
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = config.clear_colour_image ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = config.colour_image_is_swapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : 
			(config.leave_as_transfer_src ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : 
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		i++;
	}

	// Colour attachment is always first (if it is used)
	colour_attachment_ref->attachment = 0; // index into renderPassInfo.pAttachments
	colour_attachment_ref->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	depth_attachment_ref->attachment = i; // index into renderPassInfo.pAttachments
	depth_attachment_ref->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	// depth attachment

	if (config.depth_mode != PIGEON_VULKAN_RENDER_PASS_DEPTH_NONE) {
		attachments[i].format = pigeon_get_vulkan_image_format(config.depth_format);
		attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;

		attachments[i].loadOp = config.depth_mode == PIGEON_VULKAN_RENDER_PASS_DEPTH_READ_ONLY ?
			VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[i].storeOp = config.depth_mode == PIGEON_VULKAN_RENDER_PASS_DEPTH_KEEP ?
			VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;

		attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		if (config.depth_mode == PIGEON_VULKAN_RENDER_PASS_DEPTH_DISCARD || 
				config.depth_mode == PIGEON_VULKAN_RENDER_PASS_DEPTH_KEEP) {

			// Previous image contents destroyed
			attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		else { // Read-only depth buffer
			attachments[i].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		}

		if (config.depth_mode == PIGEON_VULKAN_RENDER_PASS_DEPTH_DISCARD) {
			// Image contents destroyed
			attachments[i].finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		else {
			// Image contents kept/unaltered
			attachments[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		}
		i++;
	}
	*attachment_count = i;
}

static void get_subpass(PigeonVulkanRenderPassConfig config, VkSubpassDescription* subpass, 
	VkSubpassDependency subpass_dependencies[2], unsigned int* subpass_dependency_count,
	VkAttachmentReference* colour_attachment_ref, VkAttachmentReference* depth_attachment_ref)
{
	assert(subpass && subpass_dependencies && subpass_dependency_count && colour_attachment_ref && depth_attachment_ref);

	subpass->pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	if (config.colour_image != PIGEON_WGI_IMAGE_FORMAT_NONE) {
		subpass->colorAttachmentCount = 1;
		subpass->pColorAttachments = colour_attachment_ref;
	}

	subpass->pDepthStencilAttachment = config.depth_mode != PIGEON_VULKAN_RENDER_PASS_DEPTH_NONE ? 
		depth_attachment_ref : VK_NULL_HANDLE;

	unsigned int i = 0;

	if (config.depth_mode == PIGEON_VULKAN_RENDER_PASS_DEPTH_READ_ONLY) {
		// Wait for depth write

		subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dependencies[0].dstSubpass = 0;
		subpass_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpass_dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpass_dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

		i++;
	}

	if (config.vertex_shader_depends_on_transfer || config.fragment_shader_depends_on_transfer) {
		// Wait for meshes etc. to be uploaded
		// TODO dstStageMask mask could be more specific if only meshes or only textures are uploaded e.g.
		subpass_dependencies[i].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dependencies[i].dstSubpass = 0;
		subpass_dependencies[i].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		subpass_dependencies[i].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		subpass_dependencies[i].dstStageMask = 
			config.vertex_shader_depends_on_transfer ?
				VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpass_dependencies[i].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		i++;
	}

	if (config.colour_image != PIGEON_WGI_IMAGE_FORMAT_NONE && config.colour_image_is_swapchain) {
		// Wait for swapchain

		subpass_dependencies[i].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dependencies[i].dstSubpass = 0;
		subpass_dependencies[i].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dependencies[i].srcAccessMask = 0;
		subpass_dependencies[i].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dependencies[i].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		i++;
	}

	*subpass_dependency_count = i;
}

ERROR_RETURN_TYPE pigeon_vulkan_make_render_pass(PigeonVulkanRenderPass * rp, PigeonVulkanRenderPassConfig config)
{
	assert(rp);
	ASSERT__1(config.depth_mode == PIGEON_VULKAN_RENDER_PASS_DEPTH_KEEP || config.colour_image != PIGEON_WGI_IMAGE_FORMAT_NONE, "No-op renderpass");

	if(config.depth_mode != PIGEON_VULKAN_RENDER_PASS_DEPTH_NONE) 
		ASSERT_1(pigeon_wgi_image_format_is_depth(config.depth_format));

	VkAttachmentDescription attachments[2] = { {0}, {0} };
	unsigned int attachment_count;
	VkAttachmentReference colour_attachment_ref = { 0 };
	VkAttachmentReference depth_attachment_ref = { 0 };

	get_attachments(config, attachments, &attachment_count, &colour_attachment_ref, &depth_attachment_ref);

	VkSubpassDescription subpass = { 0 };
	VkSubpassDependency subpass_dependencies[3] = { {0}, {0}, {0} };
	unsigned int subpass_dependency_count;

	get_subpass(config , &subpass, subpass_dependencies, &subpass_dependency_count, &colour_attachment_ref, &depth_attachment_ref);

	VkRenderPassCreateInfo render_pass_create = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	render_pass_create.attachmentCount = attachment_count;
	render_pass_create.pAttachments = attachments;
	render_pass_create.subpassCount = 1;
	render_pass_create.pSubpasses = &subpass;
	render_pass_create.dependencyCount = subpass_dependency_count;
	render_pass_create.pDependencies = subpass_dependencies;


	ASSERT__1(vkCreateRenderPass(vkdev, &render_pass_create, NULL, &rp->vk_renderpass) == VK_SUCCESS, "vkCreateRenderPass error");

	rp->has_colour_image = config.colour_image != PIGEON_WGI_IMAGE_FORMAT_NONE;
	rp->has_depth_image = config.depth_mode != PIGEON_VULKAN_RENDER_PASS_DEPTH_NONE;

	return 0;
}

void pigeon_vulkan_destroy_render_pass(PigeonVulkanRenderPass * rp)
{
	if (rp && rp->vk_renderpass) {
		vkDestroyRenderPass(vkdev, rp->vk_renderpass, NULL);
		rp->vk_renderpass = NULL;
	}
	else {
		assert(false);
	}
}
