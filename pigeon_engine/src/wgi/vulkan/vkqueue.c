#include "singleton.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pigeon/util.h>


ERROR_RETURN_TYPE pigeon_create_vulkan_logical_device_and_queues(void)
{
	VkDeviceQueueCreateInfo queue_create_infos[2] = { {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO},{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO} };

	// General queue

	queue_create_infos[0].queueFamilyIndex = singleton_data.general_queue_family;
	queue_create_infos[0].queueCount = 1;

	float queue_priority = 1.0f;
	queue_create_infos[0].pQueuePriorities = &queue_priority;


	// Transfer queue

	queue_create_infos[1].queueFamilyIndex = singleton_data.transfer_queue_family;
	queue_create_infos[1].queueCount = 1;
	queue_create_infos[1].pQueuePriorities = &queue_priority;

	/* Create device */

	VkPhysicalDeviceFeatures physical_device_features = {0};
	physical_device_features.depthClamp = singleton_data.depth_clamp_supported;
	physical_device_features.samplerAnisotropy = singleton_data.anisotropy_supported;
	physical_device_features.multiDrawIndirect = 1;
	physical_device_features.shaderStorageImageExtendedFormats = 1;

	VkDeviceCreateInfo device_create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	device_create_info.pQueueCreateInfos = queue_create_infos;
	device_create_info.queueCreateInfoCount = singleton_data.transfer_queue_family == UINT32_MAX ? 1 : 2;
	device_create_info.pEnabledFeatures = &physical_device_features;

	const char * extensions[3] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME
	};

	device_create_info.enabledExtensionCount = singleton_data.dedicated_allocation_supported ? 3 : 1;
	device_create_info.ppEnabledExtensionNames = extensions;

#ifndef NDEBUG
	device_create_info.enabledLayerCount = 1;
	const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
	device_create_info.ppEnabledLayerNames = &validation_layer_name;
#endif

	ASSERT__1(vkCreateDevice(singleton_data.physical_device, &device_create_info, NULL, &singleton_data.device) == VK_SUCCESS, "vkCreateDevice error");

	/* Create queues */

	vkGetDeviceQueue(singleton_data.device, singleton_data.general_queue_family, 0, &singleton_data.general_queue);

	if (singleton_data.transfer_queue_family != UINT32_MAX) {
		vkGetDeviceQueue(singleton_data.device, singleton_data.transfer_queue_family, 0, &singleton_data.transfer_queue);
	}

	return 0;

}
