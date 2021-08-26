#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <pigeon/wgi/vulkan/image.h>

typedef struct SingletonData
{
	VkInstance instance;

	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;
	bool dedicated_allocation_supported;
	VkSurfaceKHR surface;
	VkPhysicalDeviceMemoryProperties memory_properties;
	
	unsigned int uniform_buffer_min_alignment;
	bool depth_clamp_supported;
	bool anisotropy_supported;
	double timer_multiplier;

	uint32_t general_queue_family;
	uint32_t transfer_queue_family;

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
