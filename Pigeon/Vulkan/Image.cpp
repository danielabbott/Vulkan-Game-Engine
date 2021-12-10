#include "Device.h"
#include "Image.h"
#include "Include.h"
#include "Device.h"
#include <limits>

using namespace std;

namespace Vulkan {
	
	VkFormat get_vulkan_image_format(WGI::ImageFormat f);

	Image::Image(Device& i, VkImage image_handle_, WGI::ImageFormat format_,
		unsigned int width_, unsigned int height_)
	: device(i), image_handle(image_handle_), format(format_), width(width_), height(height_)
	{
		free_image_in_destructor = false;
	}

	Image::Image(Device& i, WGI::ImageFormat format_, unsigned int width_, unsigned int height_,
		WGI::MemoryCriteria& criteria, bool host_memory_texture)
	: device(i), format(format_), width(width_), height(height_)
	{
		VkImageCreateInfo image_create_info { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		image_create_info.imageType = VK_IMAGE_TYPE_2D;
		image_create_info.extent.width = width;
		image_create_info.extent.height = height;
		image_create_info.extent.depth = 1;
		image_create_info.mipLevels = 1;
		image_create_info.arrayLayers = 1;
		image_create_info.format = get_vulkan_image_format(format);
		image_create_info.tiling = host_memory_texture ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
		image_create_info.initialLayout = host_memory_texture ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;

		if(format == WGI::ImageFormat::DepthF32) {
			image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		else if(format == WGI::ImageFormat::RGBALinearF16 or format == WGI::ImageFormat::U8) {
			image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		else if(format == WGI::ImageFormat::SRGBU8) {
			image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		else {
			image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

		VkImage img;
		assert__(vkCreateImage(device.get_device_handle_(), &image_create_info, nullptr, &img) == VK_SUCCESS, "vkCreateImage error");
		image_handle = img;

		VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device.get_device_handle_(), img, &memory_requirements);

        // WGI::MemoryCriteria criteria;
        // criteria.device_local = MemoryTypePerference::Must;
        // criteria.host_visible = MemoryTypePerference::PreferedNot;
        // criteria.host_coherent = MemoryTypePerference::PreferedNot;
        // criteria.host_cached = MemoryTypePerference::PreferedNot;

        criteria.alignment = static_cast<uint32_t>(memory_requirements.alignment);
        criteria.memory_type_bits = memory_requirements.memoryTypeBits;

        assert_(memory_requirements.size <= numeric_limits<uint32_t>::max());
        memory_size_requirement = criteria.size = static_cast<uint32_t>(memory_requirements.size);
	}

	void Image::bind_memory(Ref<MemoryAllocation> memory_to_use, WGI::MemoryRegion region)
    {
        assert_(region.size >= memory_size_requirement);
        assert__(vkBindImageMemory(device.get_device_handle_(), image_handle, memory_to_use->get_handle(), region.start) == VK_SUCCESS, "vkBindImageMemory error");
        memory = memory_to_use;
        memory_properties = memory.value()->get_properties();
        memory_region = region;
    }

	Image::~Image()
	{
		if (free_image_in_destructor) {
			vkDestroyImage(device.get_device_handle_(), image_handle, nullptr);
			image_handle = nullptr;
		}
	}

}