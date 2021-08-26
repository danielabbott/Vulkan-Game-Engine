#include "singleton.h"
#include <pigeon/wgi/vulkan/image.h>
#include <pigeon/wgi/image_formats.h>
#include <pigeon/wgi/vulkan/memory.h>
#include <pigeon/util.h>
#include <assert.h>

VkFormat pigeon_get_vulkan_image_format(PigeonWGIImageFormat f)
{
	switch (f) {
	case PIGEON_WGI_IMAGE_FORMAT_NONE:
		return VK_FORMAT_UNDEFINED;



		case PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case PIGEON_WGI_IMAGE_FORMAT_BGRA_U8_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;
		case PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32:
			return VK_FORMAT_D32_SFLOAT;
		case PIGEON_WGI_IMAGE_FORMAT_DEPTH_U24:
			return VK_FORMAT_X8_D24_UNORM_PACK32;
		case PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR:
			return VK_FORMAT_R8_UNORM;
		case PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR: // TODO I8I8?
			return VK_FORMAT_R8G8_UNORM; // TODO snorm?
		case PIGEON_WGI_IMAGE_FORMAT_BC1_SRGB:
			return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		case PIGEON_WGI_IMAGE_FORMAT_BC3_SRGB:
			return VK_FORMAT_BC3_SRGB_BLOCK;
		case PIGEON_WGI_IMAGE_FORMAT_BC5_SRGB:
			return VK_FORMAT_BC5_UNORM_BLOCK; // TODO snorm?
		case PIGEON_WGI_IMAGE_FORMAT_BC7_SRGB:
			return VK_FORMAT_BC7_SRGB_BLOCK;
		case PIGEON_WGI_IMAGE_FORMAT_ETC1_LINEAR:
			return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK; // Backwards compatible with OpenGLES 2
		case PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB:
			return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
		case PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB_ALPHA_SRGB:
			return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
	}
	assert(false);
	return VK_FORMAT_UNDEFINED;
}


int pigeon_vulkan_create_image(PigeonVulkanImage* image, PigeonWGIImageFormat format,
	unsigned width, unsigned int height, unsigned int layers, unsigned int mip_levels,
	bool linear_tiling, bool preinitialised,
	bool to_be_sampled, bool to_be_attatched,
	bool to_be_transfer_src, bool to_be_transfer_dst,
	PigeonVulkanMemoryRequirements* memory_requirements)
{
	assert(image && format && width && height && layers && memory_requirements);

	ASSERT__1(width <= 16384 && height <= 16384 && layers <= 2048, "Image dimensions too large");

	VkImageCreateInfo image_create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.extent.width = width;
	image_create_info.extent.height = height;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = mip_levels;
	image_create_info.arrayLayers = layers;
	image_create_info.format = pigeon_get_vulkan_image_format(format);
	image_create_info.tiling = linear_tiling ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
	image_create_info.initialLayout = preinitialised ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

	if (to_be_sampled) {
		image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	if (to_be_attatched) {
		if (PIGEON_WGI_IMAGE_FORMAT_is_depth(format)) {
			image_create_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		else {
			image_create_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
	}

	if (to_be_transfer_src) {
		image_create_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	if (to_be_transfer_dst) {
		image_create_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

	ASSERT__1(vkCreateImage(vkdev, &image_create_info, NULL, &image->vk_image) == VK_SUCCESS, "vkCreateImage error");
	
	image->format = format;
	image->width = width;
	image->height = height;
	image->layers = layers;
	image->mip_levels = mip_levels;


	if(singleton_data.dedicated_allocation_supported) {
		VkMemoryDedicatedRequirements dedicated_requirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};

		VkMemoryRequirements2 vk_memory_requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
		vk_memory_requirements.pNext = &dedicated_requirements;

		VkImageMemoryRequirementsInfo2 image_requirements_info = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};
		image_requirements_info.image = image->vk_image;

		vkGetImageMemoryRequirements2(vkdev, &image_requirements_info, &vk_memory_requirements);

		memory_requirements->alignment = (unsigned int)vk_memory_requirements.memoryRequirements.alignment;
		memory_requirements->memory_type_bits = vk_memory_requirements.memoryRequirements.memoryTypeBits;
		memory_requirements->size = vk_memory_requirements.memoryRequirements.size;
		memory_requirements->requires_dedicated = dedicated_requirements.requiresDedicatedAllocation;
		memory_requirements->prefers_dedicated = dedicated_requirements.prefersDedicatedAllocation;
		image->requires_dedicated_memory = dedicated_requirements.requiresDedicatedAllocation;
	}
	else {
		image->requires_dedicated_memory = false;

		VkMemoryRequirements vk_memory_requirements;
		vkGetImageMemoryRequirements(vkdev, image->vk_image, &vk_memory_requirements);

		memory_requirements->alignment = (unsigned int)vk_memory_requirements.alignment;
		memory_requirements->memory_type_bits = vk_memory_requirements.memoryTypeBits;
		memory_requirements->size = vk_memory_requirements.size;
		memory_requirements->requires_dedicated = false;
		memory_requirements->prefers_dedicated = false;
	}

	return 0;
}

int pigeon_vulkan_image_bind_memory(PigeonVulkanImage* image, PigeonVulkanMemoryAllocation* memory,
	unsigned int offset)
{
	ASSERT_1(image && image->vk_image && memory && memory->vk_device_memory && !memory->is_dedicated);
	ASSERT_1(!image->requires_dedicated_memory);
	ASSERT__1(vkBindImageMemory(vkdev, image->vk_image, memory->vk_device_memory, offset) == VK_SUCCESS, "vkBindImageMemory error");
	return 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_image_bind_memory_dedicated(PigeonVulkanImage* image, PigeonVulkanMemoryAllocation* memory)
{
	ASSERT_1(image && image->vk_image && memory && memory->vk_device_memory && memory->is_dedicated);
	ASSERT__1(vkBindImageMemory(vkdev, image->vk_image, memory->vk_device_memory, 0) == VK_SUCCESS, "vkBindImageMemory error");
	return 0;
}

void pigeon_vulkan_destroy_image(PigeonVulkanImage* image)
{
	if (image && image->vk_image) {
		vkDestroyImage(vkdev, image->vk_image, NULL);
		image->vk_image = NULL;
	}
	else {
		assert(false);
	}
}


ERROR_RETURN_TYPE pigeon_vulkan_create_image_view(PigeonVulkanImageView* image_view, PigeonVulkanImage* image,
	bool array_texture)
{
	VkImageViewCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	create_info.image = image->vk_image;

	create_info.viewType = array_texture ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;

	create_info.format = pigeon_get_vulkan_image_format(image->format);

	if (PIGEON_WGI_IMAGE_FORMAT_is_depth(image->format)) {
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else {
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	create_info.subresourceRange.levelCount = image->mip_levels;
	create_info.subresourceRange.layerCount = image->layers;

	
	ASSERT__1(vkCreateImageView(vkdev, &create_info, NULL, &image_view->vk_image_view) == VK_SUCCESS, "vkCreateImageView error");
	
	
	image_view->width = image->width;
	image_view->height = image->height;
	image_view->layers = image->layers;
	image_view->format = image->format;
	image_view->mip_levels = image->mip_levels;

	return 0;
}

void pigeon_vulkan_destroy_image_view(PigeonVulkanImageView* image_view)
{
	if (image_view && image_view->vk_image_view) {
		vkDestroyImageView(vkdev, image_view->vk_image_view, NULL);
		image_view->vk_image_view = NULL;
	}
	else {
		assert(false);
	}
}
