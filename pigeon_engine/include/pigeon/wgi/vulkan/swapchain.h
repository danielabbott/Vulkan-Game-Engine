#pragma once

#include "../image_formats.h"
#include "image.h"
#include "fence.h"
#include "semaphore.h"

// Re(create) swapchain
// Returns 2 (fail) if the window is minimised or smaller than 16x16 pixels
PIGEON_ERR_RET pigeon_vulkan_create_swapchain(void);
void pigeon_vulkan_destroy_swapchain(void);


typedef struct PigeonVulkanSwapchainInfo {
	unsigned int image_count;
	PigeonWGIImageFormat format; //PIGEON_WGI_IMAGE_FORMAT_BGRA_U8_SRGB
	unsigned int width, height;
} PigeonVulkanSwapchainInfo;

PigeonVulkanSwapchainInfo pigeon_vulkan_get_swapchain_info(void);
PigeonVulkanImageView * pigeon_vulkan_get_swapchain_image_view(unsigned int i);

// 0 = Success, 1 = Error, 2 = Swapchain must be recreated (not error), 3 = No image available yet (only for block=false)
int pigeon_vulkan_next_swapchain_image(unsigned int * new_image_index, 
	PigeonVulkanSemaphore* signal, PigeonVulkanFence*, bool block);

// 0 = Success, 1 = Error, 2 = Recreate swapchain (not error)
PIGEON_ERR_RET pigeon_vulkan_swapchain_present(PigeonVulkanSemaphore * wait_semaphore);
