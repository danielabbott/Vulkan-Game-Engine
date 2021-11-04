#define VULKAN_C_
#include "singleton.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pigeon/wgi/window.h>
#include <pigeon/wgi/vulkan/vulkan.h>
#include <pigeon/assert.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifndef NDEBUG
static const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
#endif

#ifndef NDEBUG
static bool validation_layers_available();
#endif

static PIGEON_ERR_RET create_vk_instance(void);

PIGEON_ERR_RET pigeon_create_vulkan_context(bool prefer_dedicated_gpu)
{
	ASSERT_R1(!create_vk_instance());

	// Create surface
	ASSERT_LOG_R1(glfwCreateWindowSurface(singleton_data.instance, pigeon_wgi_get_glfw_window_handle(), 
		NULL, &singleton_data.surface) == VK_SUCCESS, "glfwCreateWindowSurface error");

	ASSERT_R1(!pigeon_find_vulkan_device(prefer_dedicated_gpu));
	ASSERT_R1(!pigeon_create_vulkan_logical_device_and_queues());

	return 0;
}

unsigned int pigeon_vulkan_get_buffer_min_alignment(void)
{
	return singleton_data.buffer_min_alignment;
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
static bool validation_layers_available(bool * validation_ext)
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, NULL);

	VkLayerProperties* all_layers = malloc(sizeof *all_layers * layer_count);
	if (!all_layers) return 1;

	vkEnumerateInstanceLayerProperties(&layer_count, all_layers);

	bool found_layer = false;

	for (unsigned int i = 0; i < layer_count; i++) {
		all_layers[i].layerName[VK_MAX_EXTENSION_NAME_SIZE - 1] = 0;
		all_layers[i].description[VK_MAX_DESCRIPTION_SIZE - 1] = 0;
		printf("Found layer: %s, %s\n", all_layers[i].layerName, all_layers[i].description);

		if (strcmp(all_layers[i].layerName, validation_layer_name) == 0) {
			found_layer = true;
			break;
		}
	}

	free(all_layers);

	if(!found_layer) return false;

	*validation_ext = false;

	uint32_t ext_count = 0;
	vkEnumerateInstanceExtensionProperties(validation_layer_name, &ext_count, NULL);

	VkExtensionProperties * ext = malloc(ext_count * sizeof *ext);
	if(ext) {
		vkEnumerateInstanceExtensionProperties(validation_layer_name, &ext_count, ext);

		for(unsigned int i = 0;  i < ext_count; i++) {
			if(strcmp(ext[i].extensionName, "VK_EXT_validation_features") == 0) {
				*validation_ext = true;
				break;
			}
		}
	}
	
	return true;
}
#endif

static PIGEON_ERR_RET create_vk_instance(void)
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

#ifdef DEBUG
	bool validation_ext = false;

	// VkValidationFeaturesEXT validation_features = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	// VkValidationFeatureEnableEXT validation_features_list[1] = {
	// 	// VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, // requires spir-v 1.1 (vk1.1)
	// 	// VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
	// 	// VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
	// 	VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
	// 	// VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
	// };
	// validation_features.enabledValidationFeatureCount = (sizeof validation_features_list) / sizeof(VkValidationFeatureEnableEXT);
	// validation_features.pEnabledValidationFeatures = validation_features_list;

	if (validation_layers_available(&validation_ext)) {
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = &validation_layer_name;
		// if(validation_ext) createInfo.pNext = &validation_features;
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


	ASSERT_LOG_R1(result != VK_ERROR_OUT_OF_HOST_MEMORY, "vkCreateInstance error: Out of Memory");
	ASSERT_LOG_R1(result != VK_ERROR_OUT_OF_DEVICE_MEMORY, "vkCreateInstance error: Out of Device Memory");
	ASSERT_LOG_R1(result != VK_ERROR_INITIALIZATION_FAILED, "vkCreateInstance error: Initialisation Failed");
	ASSERT_LOG_R1(result != VK_ERROR_LAYER_NOT_PRESENT, "vkCreateInstance error: Validation Layer not Present");
	ASSERT_LOG_R1(result != VK_ERROR_EXTENSION_NOT_PRESENT, "vkCreateInstance error: Extension not Present");
	ASSERT_LOG_R1(result != VK_ERROR_INCOMPATIBLE_DRIVER, "vkCreateInstance error: Incompatible Driver");
	ASSERT_LOG_R1(result == VK_SUCCESS, "vkCreateInstance error");


	return 0;
}
