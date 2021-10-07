#define VULKAN_C_
#include "singleton.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pigeon/wgi/window.h>
#include <pigeon/wgi/vulkan/vulkan.h>
#include <pigeon/util.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifndef NDEBUG
static const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
#endif

#ifndef NDEBUG
static bool validation_layers_available();
#endif

static ERROR_RETURN_TYPE create_vk_instance(void);

ERROR_RETURN_TYPE pigeon_create_vulkan_context(bool prefer_dedicated_gpu)
{
	ASSERT_1(!create_vk_instance());

	// Create surface
	ASSERT__1(glfwCreateWindowSurface(singleton_data.instance, pigeon_wgi_get_glfw_window_handle(), 
		NULL, &singleton_data.surface) == VK_SUCCESS, "glfwCreateWindowSurface error");

	ASSERT_1(!pigeon_find_vulkan_device(prefer_dedicated_gpu));
	ASSERT_1(!pigeon_create_vulkan_logical_device_and_queues());

	return 0;
}

unsigned int pigeon_vulkan_get_uniform_buffer_min_alignment(void)
{
	return singleton_data.uniform_buffer_min_alignment;
}

bool pigeon_vulkan_compact_hdr_framebuffer_available(void)
{
	return singleton_data.b10g11r11_ufloat_pack32_optimal_available;
}

bool pigeon_vulkan_bc1_optimal_available(void)
{
	return singleton_data.bc1_optimal_available;
}
bool pigeon_vulkan_bc3_optimal_available(void)
{
	return singleton_data.bc3_optimal_available;
}
bool pigeon_vulkan_bc5_optimal_available(void)
{
	return singleton_data.bc5_optimal_available;
}
bool pigeon_vulkan_bc7_optimal_available(void)
{
	return singleton_data.bc7_optimal_available;
}
bool pigeon_vulkan_etc1_optimal_available(void)
{
	return singleton_data.etc1_optimal_available;
}
bool pigeon_vulkan_etc2_optimal_available(void)
{
	return singleton_data.etc2_optimal_available;
}
bool pigeon_vulkan_etc2_rgba_optimal_available(void)
{
	return singleton_data.etc2_rgba_optimal_available;
}


void pigeon_vulkan_wait_idle(void)
{
	if(vkdev) vkDeviceWaitIdle(vkdev);
}

void pigeon_destroy_vulkan_context(void)
{
	if(!vkdev) return;

	vkDeviceWaitIdle(vkdev);
	if(singleton_data.device) vkDestroyDevice(singleton_data.device, NULL);
	if(singleton_data.surface) vkDestroySurfaceKHR(singleton_data.instance, singleton_data.surface, NULL);
	if(singleton_data.instance) vkDestroyInstance(singleton_data.instance, NULL);

	singleton_data.instance = NULL;
	singleton_data.surface = NULL;
	singleton_data.device = NULL;
}

#ifndef NDEBUG
static bool validation_layers_available(void)
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, NULL);

	VkLayerProperties* all_layers = malloc(sizeof *all_layers * layer_count);
	if (!all_layers) return 1;

	vkEnumerateInstanceLayerProperties(&layer_count, all_layers);


	for (unsigned int i = 0; i < layer_count; i++) {
		all_layers[i].layerName[VK_MAX_EXTENSION_NAME_SIZE - 1] = 0;
		all_layers[i].description[VK_MAX_DESCRIPTION_SIZE - 1] = 0;
		printf("Found layer: %s, %s\n", all_layers[i].layerName, all_layers[i].description);

		if (strcmp(all_layers[i].layerName, validation_layer_name) == 0) {
			free(all_layers);
			return true;
		}
	}


	free(all_layers);
	return false;
}
#endif

static ERROR_RETURN_TYPE create_vk_instance(void)
{
	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = "Pigeon-powered Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Pigeon";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;


	createInfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&createInfo.enabledExtensionCount);

#ifndef NDEBUG
	if (validation_layers_available()) {
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = &validation_layer_name;
	}
	else {
		puts("Validation layers not found. Force enable them using Vulkan Configurator (vkconfig)");
	}
#endif

	singleton_data.dedicated_allocation_supported = true;

	VkResult result = vkCreateInstance(&createInfo, NULL, &singleton_data.instance);
	
	if(result == VK_ERROR_INCOMPATIBLE_DRIVER) {
		singleton_data.dedicated_allocation_supported = false;
		appInfo.apiVersion = VK_API_VERSION_1_0;
		result = vkCreateInstance(&createInfo, NULL, &singleton_data.instance);
	}


	ASSERT__1(result != VK_ERROR_OUT_OF_HOST_MEMORY, "vkCreateInstance error: Out of Memory");
	ASSERT__1(result != VK_ERROR_OUT_OF_DEVICE_MEMORY, "vkCreateInstance error: Out of Device Memory");
	ASSERT__1(result != VK_ERROR_INITIALIZATION_FAILED, "vkCreateInstance error: Initialisation Failed");
	ASSERT__1(result != VK_ERROR_LAYER_NOT_PRESENT, "vkCreateInstance error: Validation Layer not Present");
	ASSERT__1(result != VK_ERROR_EXTENSION_NOT_PRESENT, "vkCreateInstance error: Extension not Present");
	ASSERT__1(result != VK_ERROR_INCOMPATIBLE_DRIVER, "vkCreateInstance error: Incompatible Driver");
	ASSERT__1(result == VK_SUCCESS, "vkCreateInstance error");


	return 0;
}
