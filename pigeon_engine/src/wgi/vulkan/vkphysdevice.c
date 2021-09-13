#include "singleton.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pigeon/util.h>


static bool is_device_swapchain_capable(VkPhysicalDevice physical_device)
{
	assert(physical_device);

	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, NULL);

	VkExtensionProperties* available_extensions = malloc(sizeof *available_extensions * extension_count);
	if (!available_extensions) return false;

	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, available_extensions);

	for (unsigned int i = 0; i < extension_count; i++) {
		if (strcmp(available_extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
			free(available_extensions);
			return true;
		}
	}


	free(available_extensions);
	return false;
}

// Returns true if found a general queue
static bool find_general_queue(VkPhysicalDevice physical_device, 
	VkQueueFamilyProperties* queue_properties, unsigned int queue_family_count,  bool compute)
{
	assert(physical_device && queue_properties);

	for (unsigned int i = 0; i < queue_family_count; i++) {
		if ((queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queue_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			 && queue_properties[i].queueCount) {

			if (!compute || (queue_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
				VkBool32 present = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, singleton_data.surface, &present);
				if (present) {
					singleton_data.general_queue_family = i;
					singleton_data.general_queue_timestamp_bits_valid = queue_properties[i].timestampValidBits;
					return true;
				}
			}
		}
	}
	return false;
}

static bool is_device_suitable(VkPhysicalDevice physical_device, bool dedicated) 
{	
	assert(physical_device);

	if (!is_device_swapchain_capable(physical_device)) {
		return false;
	}

	/* Queues */

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

	VkQueueFamilyProperties* queue_properties = malloc(sizeof *queue_properties * queue_family_count);

	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_properties);

	/* Find general queue */

	singleton_data.general_queue_family = UINT32_MAX;
	bool found_general_queue = find_general_queue(physical_device, queue_properties, queue_family_count, true);

	if (!found_general_queue) {
		found_general_queue = find_general_queue(physical_device, queue_properties, queue_family_count, false);

		if (!found_general_queue) {
			return false;
		}
	}

	/* Find dedicated transfer queue */

	singleton_data.transfer_queue_family = UINT32_MAX;
	for (unsigned int i = 0; i < queue_family_count; i++) {
		if ((queue_properties[i].queueFlags & 7) == VK_QUEUE_TRANSFER_BIT && queue_properties[i].queueCount) {
			singleton_data.transfer_queue_family = i;
			singleton_data.transfer_queue_timestamp_bits_valid = queue_properties[i].timestampValidBits;
		}
	}

	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;

	vkGetPhysicalDeviceProperties(physical_device, &device_properties);

	if ((device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) != dedicated) {
		return false;
	}

	vkGetPhysicalDeviceFeatures(physical_device, &device_features);

	if(!device_features.multiDrawIndirect) {
		fputs("Multidraw unsupported\n", stderr);
		return false;
	}

	singleton_data.depth_clamp_supported = device_features.depthClamp;
	singleton_data.anisotropy_supported = device_features.samplerAnisotropy && device_properties.limits.maxSamplerAnisotropy >= 16;
	


	singleton_data.timer_multiplier = ((double)device_properties.limits.timestampPeriod / 1000.0) / 1000.0;

	singleton_data.uniform_buffer_min_alignment = (unsigned)device_properties.limits.minUniformBufferOffsetAlignment;
	
	vkGetPhysicalDeviceMemoryProperties(physical_device, &singleton_data.memory_properties);


	VkFormatProperties format_properties;

	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_B10G11R11_UFLOAT_PACK32, &format_properties);
	singleton_data.b10g11r11_ufloat_pack32_optimal_available = 
		(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT) != 0;

	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_BC1_RGB_SRGB_BLOCK, &format_properties);
	singleton_data.bc1_optimal_available = 
		(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;

	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_BC3_SRGB_BLOCK, &format_properties);
	singleton_data.bc3_optimal_available = 
		(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;

	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_BC5_UNORM_BLOCK, &format_properties);
	singleton_data.bc5_optimal_available = 
		(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;

	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_BC7_SRGB_BLOCK, &format_properties);
	singleton_data.bc7_optimal_available = 
		(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;

	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, &format_properties);
	singleton_data.etc1_optimal_available = 
		(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;

	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, &format_properties);
	singleton_data.etc2_optimal_available = 
		(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;

	vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, &format_properties);
	singleton_data.etc2_rgba_optimal_available = 
		(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;


	
	printf("Using device: %s\n", device_properties.deviceName);

	if (singleton_data.depth_clamp_supported) puts("Depth clamp supported");
	if (singleton_data.anisotropy_supported) puts("Anisotropic filtering supported");
	if (singleton_data.dedicated_allocation_supported) puts("Dedicated allocation supported");
	if (singleton_data.b10g11r11_ufloat_pack32_optimal_available) puts("VK_FORMAT_B10G11R11_UFLOAT_PACK32 render+blend target supported");

	if (singleton_data.bc1_optimal_available) puts("BC1 supported");
	if (singleton_data.bc3_optimal_available) puts("BC3 supported");
	if (singleton_data.bc5_optimal_available) puts("BC5 supported");
	if (singleton_data.bc7_optimal_available) puts("BC7 supported");
	if (singleton_data.etc1_optimal_available) puts("ETC1 supported");
	if (singleton_data.etc2_optimal_available) puts("ETC2 RGB supported");
	if (singleton_data.etc2_rgba_optimal_available) puts("ETC2 RGBA supported");

	return true;
}


ERROR_RETURN_TYPE pigeon_find_vulkan_device(bool prefer_dedicated_gpu)
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(singleton_data.instance, &device_count, NULL);

	ASSERT__1(device_count != 0, "No GPUs");

	VkPhysicalDevice* devices = malloc(sizeof *devices * device_count);
	if (!devices) return 1;


	vkEnumeratePhysicalDevices(singleton_data.instance, &device_count, devices);

	// Find prefered graphics device type (dedicated/integrated)
	for (unsigned int i = 0; i < device_count; i++) {
		if (is_device_suitable(devices[i], prefer_dedicated_gpu)) {
			singleton_data.physical_device = devices[i];
			break;
		}
	}

	if (!singleton_data.physical_device) {
		// Settle for the other option (integrated/dedicated)
		for (unsigned int i = 0; i < device_count; i++) {
			if (is_device_suitable(devices[i], !prefer_dedicated_gpu)) {
				singleton_data.physical_device = devices[i];
				break;
			}
		}
	}

	free(devices);

	ASSERT__1(singleton_data.physical_device, "No suitable GPU found");



	return 0;

}
