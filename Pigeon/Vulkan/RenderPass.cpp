#include "Device.h"
#include "RenderPass.h"
#include "Include.h"
#include "../BetterAssert.h"

using namespace std;

namespace Vulkan {
	VkFormat get_vulkan_image_format(WGI::ImageFormat f);

	RenderPass::RenderPass(Device& device_, CreateInfo info)
		: device(device_),
		has_colour_image_(info.colour_image.has_value()),
		has_depth_image_(info.depth != DepthMode::None)
	{
		assert__(info.depth == DepthMode::Keep or info.colour_image.has_value(), "No-op renderpass");


		vector<VkAttachmentDescription> attachments;

		if(info.colour_image.has_value()) {
			// Swap chain image
			VkAttachmentDescription attachment{};
			attachment.format = get_vulkan_image_format(info.colour_image.value());
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = info.clear_colour_image ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachment.finalLayout = info.colour_image_is_swapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachments.push_back(attachment);
		}

		VkAttachmentReference colour_attachment_ref = {};
		colour_attachment_ref.attachment = 0; // index into renderPassInfo.pAttachments
		colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref = {};
		depth_attachment_ref = VkAttachmentReference{};
		depth_attachment_ref.attachment = attachments.size(); // index into renderPassInfo.pAttachments
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		// Depth buffer
		if(info.depth != DepthMode::None) {
			VkAttachmentDescription attachment{};
			attachment.format = VK_FORMAT_D32_SFLOAT;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;

			attachment.loadOp = info.depth == DepthMode::ReadOnly ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.storeOp = info.depth == DepthMode::Keep ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			if(info.depth == DepthMode::Discard or info.depth == DepthMode::Keep) {
				// Image contents destroyed
				attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			}
			else { // Read-only depth buffer
				attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}

			if(info.depth == DepthMode::Discard) {
				// Image contents destroyed
				attachment.finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			}
			else {
				// Image contents kept/unaltered
				attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}

			
			attachments.push_back(attachment);
		}




		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		if(info.colour_image.has_value()) {
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colour_attachment_ref;
		}

		subpass.pDepthStencilAttachment = info.depth != DepthMode::None ? &depth_attachment_ref : VK_NULL_HANDLE;

		vector<VkSubpassDependency> dependencies;

		if (info.depth == DepthMode::ReadOnly) {
			// Wait for depth write
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			

			dependencies.push_back(dependency);
		}


		if(info.depends_on_transfer) {
			// Wait for meshes etc. to be uploaded
			// TODO Stage mask could be more specific if only meshes or only textures are uploaded e.g.
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dependency.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			dependency.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies.push_back(dependency);
		}

		if(info.colour_image.has_value() and info.colour_image_is_swapchain) {
			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies.push_back(dependency);
		}

		VkRenderPassCreateInfo render_pass_create{};
		render_pass_create.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create.attachmentCount = attachments.size();
		render_pass_create.pAttachments = attachments.data();
		render_pass_create.subpassCount = 1;
		render_pass_create.pSubpasses = &subpass;
		render_pass_create.dependencyCount = dependencies.size();
		render_pass_create.pDependencies = dependencies.data();

		assert__(vkCreateRenderPass(device.get_device_handle_(),
			&render_pass_create, nullptr, &render_pass) == VK_SUCCESS, "vkCreateRenderPass error");
	}

	RenderPass::~RenderPass() {
		vkDestroyRenderPass(device.get_device_handle_(), render_pass, nullptr);
	}
}