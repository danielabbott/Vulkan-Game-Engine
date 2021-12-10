#include "Device.h"
#include "Framebuffer.h"
#include "Include.h"
#include "../BetterAssert.h"

using namespace std;

namespace Vulkan {

	Framebuffer::Framebuffer(RenderPass const& render_pass, Ref<ImageView> image_view_, Ref<ImageView> depth_)
		:image_view(move(image_view_)), depth_image_view(move(depth_))
	{
		assert__(image_view or depth_image_view, "Framebuffer with no attachments");

		VkFramebufferCreateInfo framebuffer_create {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		framebuffer_create.renderPass = render_pass.get_handle();

		vector<VkImageView> attachments;
		if(image_view) {
			attachments.push_back(image_view->get_handle());
		}
		if(depth_image_view) {
			attachments.push_back(depth_image_view->get_handle());
		}

		framebuffer_create.pAttachments = attachments.data();
		framebuffer_create.attachmentCount = attachments.size();

		VkDevice vulkan_device;

		if(image_view) {
			framebuffer_create.width = image_view->get_image()->get_width();
			framebuffer_create.height = image_view->get_image()->get_height();
			vulkan_device = image_view->get_image()->get_device().get_device_handle_();

			if(depth_image_view) {
				assert__(vulkan_device == depth_image_view->get_image()->get_device().get_device_handle_(),
					"Framebuffer depth and colour image devices do not match");
			}
		}
		else {
			framebuffer_create.width = depth_image_view->get_image()->get_width();
			framebuffer_create.height = depth_image_view->get_image()->get_height();

			vulkan_device = depth_image_view->get_image()->get_device().get_device_handle_();
		}
		framebuffer_create.layers = 1;

		if(image_view and depth_image_view) {
			assert__(
				image_view->get_image()->get_width() == depth_image_view->get_image()->get_width() and
				image_view->get_image()->get_height() == depth_image_view->get_image()->get_height(), 
				"Framebuffer depth and colour image dimensions do not match"
			);
		}

		assert__(vkCreateFramebuffer(vulkan_device, &framebuffer_create, nullptr, &handle) == VK_SUCCESS, 
					"vkCreateFramebuffer error");
	}

	Framebuffer::~Framebuffer()
	{
		vkDestroyFramebuffer(
			image_view ?
				image_view->get_image()->get_device().get_device_handle_() :
				depth_image_view->get_image()->get_device().get_device_handle_(), 
			handle, nullptr
		);
	}
}