#pragma once

#include <pigeon/wgi/image_formats.h>
#include <pigeon/wgi/vulkan/memory.h>


struct VkImage_T;
struct VkImageView_T;

typedef struct PigeonVulkanImage
{
	struct VkImage_T* vk_image;
	PigeonWGIImageFormat format;
	unsigned int width, height, layers, mip_levels;
	bool requires_dedicated_memory;
} PigeonVulkanImage;

int pigeon_vulkan_create_image(PigeonVulkanImage*, PigeonWGIImageFormat,
	unsigned width, unsigned int height, unsigned int layers, unsigned int mip_levels,
	bool linear_tiling, // true = image is 2D array of pixels, for read/modify on CPU
	bool preinitialised, // Image data will not be wiped on first use by GPU
	bool to_be_sampled, // Used as texture
	bool to_be_attatched, // Used as framebufer (depth or colour)
	bool to_be_transfer_src, bool to_be_transfer_dst,
	PigeonVulkanMemoryRequirements*);

int pigeon_vulkan_image_bind_memory(PigeonVulkanImage*, PigeonVulkanMemoryAllocation*, 
	unsigned int offset);
ERROR_RETURN_TYPE pigeon_vulkan_image_bind_memory_dedicated(PigeonVulkanImage*, PigeonVulkanMemoryAllocation*);
void pigeon_vulkan_destroy_image(PigeonVulkanImage*);


typedef struct PigeonVulkanImageView
{
	struct VkImageView_T* vk_image_view;
	unsigned int width, height, layers, mip_levels;
	PigeonWGIImageFormat format;
} PigeonVulkanImageView;

ERROR_RETURN_TYPE pigeon_vulkan_create_image_view(PigeonVulkanImageView*, PigeonVulkanImage*, bool array_texture);
void pigeon_vulkan_destroy_image_view(PigeonVulkanImageView*);


// Convenience function

ERROR_RETURN_TYPE pigeon_vulkan_create_texture_with_dedicated_memory(PigeonVulkanImage * image, 
	PigeonVulkanMemoryAllocation * memory, PigeonVulkanImageView * image_view,
	PigeonWGIImageFormat format, unsigned int width, unsigned int height,
	unsigned int layers, unsigned int mip_maps, bool device_local);
