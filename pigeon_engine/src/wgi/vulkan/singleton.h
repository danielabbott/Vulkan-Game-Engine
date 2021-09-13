#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <pigeon/wgi/vulkan/image.h>

typedef struct SingletonData
{
	VkInstance instance;

	VkPhysicalDevice physical_device;
	bool dedicated_allocation_supported;
	VkSurfaceKHR surface;
	VkPhysicalDeviceMemoryProperties memory_properties;
	
	unsigned int uniform_buffer_min_alignment;
	bool depth_clamp_supported;
	bool anisotropy_supported;
	double timer_multiplier;
	bool b10g11r11_ufloat_pack32_optimal_available;
	bool bc1_optimal_available;
	bool bc3_optimal_available;
	bool bc5_optimal_available;
	bool bc7_optimal_available;
	bool etc1_optimal_available;
	bool etc2_optimal_available;
	bool etc2_rgba_optimal_available;

	uint32_t general_queue_family;
	uint32_t transfer_queue_family;

	unsigned int general_queue_timestamp_bits_valid;
	unsigned int transfer_queue_timestamp_bits_valid;

	VkQueue general_queue;
	VkQueue transfer_queue;
	VkDevice device;

	unsigned int swapchain_image_count;
	unsigned int swapchain_width;
	unsigned int swapchain_height;

	VkSurfaceCapabilitiesKHR surface_capabilities;
	VkSwapchainKHR swapchain_handle;
	unsigned int current_swapchain_image;

	VkImage* swapchain_images;
	PigeonVulkanImageView* swapchain_image_views;
} SingletonData;

#ifndef VULKAN_C_
extern
#endif
SingletonData pigeon_vulkan_singleton_data;

#define singleton_data pigeon_vulkan_singleton_data
#define vkdev singleton_data.device


/* Functions used during device initialisation */

ERROR_RETURN_TYPE pigeon_find_vulkan_device(bool prefer_dedicated_gpu);
ERROR_RETURN_TYPE pigeon_create_vulkan_logical_device_and_queues(void);

VkFormat pigeon_get_vulkan_image_format(PigeonWGIImageFormat);
