#include "Device.h"
#include "ImageView.h"

#include "Include.h"
#include "../BetterAssert.h"

namespace Vulkan {
	VkFormat get_vulkan_image_format(WGI::ImageFormat f) {
		switch(f) {
			case WGI::ImageFormat::RGBALinearU8:
				return VK_FORMAT_B8G8R8A8_UNORM;
			case WGI::ImageFormat::SRGBU8:
				return VK_FORMAT_B8G8R8A8_SRGB;
			case WGI::ImageFormat::DepthF32:
				return VK_FORMAT_D32_SFLOAT;
			case WGI::ImageFormat::RGBALinearF16:
				return VK_FORMAT_R16G16B16A16_SFLOAT;
			case WGI::ImageFormat::U8:
				return VK_FORMAT_R8_UNORM;
		}
		assert_(false);
		return VK_FORMAT_B8G8R8A8_SRGB;
	}

	ImageView::ImageView(Ref<Image> image_)
		:image(image_)
	{
		VkImageViewCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = image->get_handle();
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = get_vulkan_image_format(image->get_format());
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		if(image->get_format() == WGI::ImageFormat::DepthF32) {
			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
 		else {
			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		assert__(vkCreateImageView(image->get_device().get_device_handle_(), &create_info, nullptr, &handle) == VK_SUCCESS, "vkCreateImageView error");
	}

	ImageView::~ImageView()
	{
		vkDestroyImageView(image->get_device().get_device_handle_(), handle, nullptr);
	}
}