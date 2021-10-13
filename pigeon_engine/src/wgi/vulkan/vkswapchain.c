#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "singleton.h"
#include <stdlib.h>
#include <pigeon/wgi/vulkan/swapchain.h>
#include <pigeon/assert.h>

extern GLFWwindow* pigeon_wgi_glfw_window;




static PIGEON_ERR_RET create_swapchain(void);
static PIGEON_ERR_RET get_images(void);

PIGEON_ERR_RET pigeon_vulkan_create_swapchain(void)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(singleton_data.physical_device, singleton_data.surface, 
		&singleton_data.surface_capabilities);

	int err = create_swapchain();
	if (err) return err;
	if (get_images()) return 1;

	return 0;
}

static PIGEON_ERR_RET create_swapchain(void)
{
	// Aim for 2 images

	unsigned int image_count = 2;
	if (singleton_data.surface_capabilities.maxImageCount == 1) {
		image_count = 1;
	}
	if (singleton_data.surface_capabilities.minImageCount > 2) {
		image_count = singleton_data.surface_capabilities.minImageCount;
	}

	VkFormat chosen_format = VK_FORMAT_B8G8R8A8_SRGB;

	uint32_t formats_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(singleton_data.physical_device, singleton_data.surface, &formats_count, NULL);

	VkSurfaceFormatKHR * formats = malloc(formats_count * sizeof *formats);
	ASSERT_R1(formats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(singleton_data.physical_device, singleton_data.surface, &formats_count, formats);

	for(unsigned int i = 0; i < formats_count; i++) {
		if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB || formats[i].format == VK_FORMAT_R8G8B8A8_SRGB) {
			chosen_format = formats[i].format;
			break;
		}
		else {
			chosen_format = formats[i].format;
		}
	}
	free(formats);


	VkSwapchainCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	create_info.surface = singleton_data.surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = chosen_format;
	create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	glfwPollEvents();
	glfwGetFramebufferSize(pigeon_wgi_glfw_window, (int*)&singleton_data.swapchain_width, (int*)&singleton_data.swapchain_height);
	if(singleton_data.swapchain_width < 16 || singleton_data.swapchain_height < 16) {
		return 2;
	}


	create_info.imageExtent.width = singleton_data.swapchain_width;
	create_info.imageExtent.height = singleton_data.swapchain_height;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

	create_info.preTransform = singleton_data.surface_capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	create_info.clipped = VK_TRUE;

	ASSERT_LOG_R1(vkCreateSwapchainKHR(vkdev, &create_info, NULL, &singleton_data.swapchain_handle) == VK_SUCCESS, "Error creating swapchain");

	return 0;
}

static PIGEON_ERR_RET get_images(void)
{
	// Images

	vkGetSwapchainImagesKHR(vkdev, singleton_data.swapchain_handle, &singleton_data.swapchain_image_count, NULL);

	singleton_data.swapchain_images = malloc(sizeof *singleton_data.swapchain_images * singleton_data.swapchain_image_count);
	if (!singleton_data.swapchain_images) return 1;

	singleton_data.swapchain_image_views = calloc(singleton_data.swapchain_image_count, sizeof * singleton_data.swapchain_image_views);
	if (!singleton_data.swapchain_image_views) {
		free(singleton_data.swapchain_images);
		singleton_data.swapchain_images = NULL;
		return 1;
	}



	vkGetSwapchainImagesKHR(vkdev, singleton_data.swapchain_handle, &singleton_data.swapchain_image_count, singleton_data.swapchain_images);

	// Image views

	for (unsigned int i = 0; i < singleton_data.swapchain_image_count; i++) {
		VkImageViewCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		create_info.image = singleton_data.swapchain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = VK_FORMAT_B8G8R8A8_SRGB;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.layerCount = 1;

		singleton_data.swapchain_image_views[i].width = singleton_data.swapchain_width;
		singleton_data.swapchain_image_views[i].height = singleton_data.swapchain_height;
		singleton_data.swapchain_image_views[i].layers = 1;

		ASSERT_LOG_R1(vkCreateImageView(vkdev, &create_info, NULL, &singleton_data.swapchain_image_views[i].vk_image_view) == VK_SUCCESS, "vkCreateImageView error");
	}
	return 0;
}


void pigeon_vulkan_destroy_swapchain(void)
{
	if(!vkdev) return;

	singleton_data.current_swapchain_image = UINT32_MAX;

	for (unsigned int i = 0; i < singleton_data.swapchain_image_count; i++) {
		if (singleton_data.swapchain_image_views && singleton_data.swapchain_image_views[i].vk_image_view) {
			pigeon_vulkan_destroy_image_view(&singleton_data.swapchain_image_views[i]);
		}
	}
	if (singleton_data.swapchain_image_views) {
		free(singleton_data.swapchain_image_views);
		singleton_data.swapchain_image_views = NULL;
	}
	if (singleton_data.swapchain_images) {
		free(singleton_data.swapchain_images);
		singleton_data.swapchain_images = NULL;
	}

	if (singleton_data.swapchain_handle) {
		vkDestroySwapchainKHR(vkdev, singleton_data.swapchain_handle, NULL);
		singleton_data.swapchain_handle = NULL;
	}

	singleton_data.swapchain_image_count = singleton_data.swapchain_width = singleton_data.swapchain_height = 0;
}


PigeonVulkanSwapchainInfo pigeon_vulkan_get_swapchain_info(void)
{
	PigeonVulkanSwapchainInfo info = {
		.image_count = singleton_data.swapchain_image_count,
		.format = PIGEON_WGI_IMAGE_FORMAT_BGRA_U8_SRGB,
		.width = singleton_data.swapchain_width,
		.height = singleton_data.swapchain_height
	};
	return info;
}

PigeonVulkanImageView * pigeon_vulkan_get_swapchain_image_view(unsigned int i)
{
	assert(i < singleton_data.swapchain_image_count);
	return &singleton_data.swapchain_image_views[i];
}


int pigeon_vulkan_next_swapchain_image(unsigned int * new_image_index, 
	PigeonVulkanSemaphore* semaphore, PigeonVulkanFence* fence, bool block)
{
	assert(semaphore || fence);

	VkResult result = vkAcquireNextImageKHR(vkdev, singleton_data.swapchain_handle, block ? 3000000000 : 0, 
	semaphore ? semaphore->vk_semaphore : NULL, fence ? fence->vk_fence : NULL, 
	&singleton_data.current_swapchain_image);

	if(result == VK_TIMEOUT) {
		if(block) {
			ERRLOG("vkAcquireNextImageKHR timeout");
			return 1;
		}
		return 3;
	}

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return 2;
	}

	ASSERT_LOG_R1((result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) && singleton_data.current_swapchain_image < 999, "vkAcquireNextImageKHR error");
	*new_image_index = singleton_data.current_swapchain_image;

	return 0;
}


PIGEON_ERR_RET pigeon_vulkan_swapchain_present(PigeonVulkanSemaphore * wait_semaphore)
{
	VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

	present_info.waitSemaphoreCount = wait_semaphore ? 1 : 0;
	present_info.pWaitSemaphores = wait_semaphore ? &wait_semaphore->vk_semaphore : NULL;

	present_info.swapchainCount = 1;
	present_info.pSwapchains = &singleton_data.swapchain_handle;
	present_info.pImageIndices = &singleton_data.current_swapchain_image;

	VkResult result = vkQueuePresentKHR(singleton_data.general_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		return 2;
	}

	ASSERT_LOG_R1(result == VK_SUCCESS, "vkQueuePresentKHR error");
	return 0;
}
