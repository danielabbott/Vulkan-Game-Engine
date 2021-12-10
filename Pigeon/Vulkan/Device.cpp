#define INCLUDE_GLFW
#include "Include.h"
#include "Device.h"
#include "../WGI/Window.h"
#include <spdlog/spdlog.h>

using namespace std;

namespace Vulkan {

	struct Device_ {
		VkPhysicalDeviceProperties device_properties = {};
		VkPhysicalDeviceFeatures device_features = {}; // Supported features, not enabled features
		VkPhysicalDeviceMemoryProperties memory_properties;
		std::vector<VkQueueFamilyProperties> queue_families;
	};
	
	VkPhysicalDeviceMemoryProperties const& Device::get_memory_properties() const
	{
		return data->memory_properties;
	}
	
	VkPhysicalDeviceMemoryProperties & Device::get_memory_properties_()
	{
		return data->memory_properties;
	}

	static VkInstance vulkan_instance = VK_NULL_HANDLE;

	VkInstance get_vulkan_instance_handle()
	{
		return vulkan_instance;
	}

	//VkDevice_T* Device::get_device_handle_() const
	//{
	//	return instance_data->device;
	//}
	//
	//VkPhysicalDevice_T* Device::get_physical_device_handle_() const
	//{
	//	return instance_data->physical_device;
	//}
	//
	//VkSurfaceKHR Device::get_surface_handle_() const
	//{
	//	return instance_data->surface;
	//}

	Ref<WGI::Window> const& Device::get_window() const
	{
		return window;
	}

	bool Device::supports_depth_clamp() const
	{
		return depth_clamp_supported;
	}

	bool Device::supports_anisotropic_filtering() const
	{
		return anisotropy_supported;
	}

	void Device::create_instance() {
		if (vulkan_instance != nullptr) {
			return;
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Pigeon";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Pigeon";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
		createInfo.pApplicationInfo = &appInfo;
		

		createInfo.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&createInfo.enabledExtensionCount);

#ifndef NDEBUG
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		vector<VkLayerProperties> all_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, all_layers.data());

		bool validation_layers_found = false;
		for (VkLayerProperties const& layer: all_layers) {
			spdlog::debug("Layer: {}, {}", layer.layerName, layer.description);

			if (strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
				validation_layers_found = true;
				break;
			}
		}

		if (validation_layers_found) {
			createInfo.enabledLayerCount = 1;
			const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
			createInfo.ppEnabledLayerNames = &validation_layer_name;
		}
		else {
			spdlog::error("Validation layers not found. Force enable them using Vulkan Configurator (vkconfig)");
		}
#endif

		auto result = vkCreateInstance(&createInfo, nullptr, &vulkan_instance);
		assert__(result != VK_ERROR_OUT_OF_HOST_MEMORY, "vkCreateInstance error: Out of Memory");
		assert__(result != VK_ERROR_OUT_OF_DEVICE_MEMORY, "vkCreateInstance error: Out of Device Memory");
		assert__(result != VK_ERROR_INITIALIZATION_FAILED, "vkCreateInstance error: Initialisation Failed");
		assert__(result != VK_ERROR_LAYER_NOT_PRESENT, "vkCreateInstance error: Validation Layer not Present");
		assert__(result != VK_ERROR_EXTENSION_NOT_PRESENT, "vkCreateInstance error: Extension not Present");
		assert__(result != VK_ERROR_INCOMPATIBLE_DRIVER, "vkCreateInstance error: Incompatible Driver");
		assert__(result == VK_SUCCESS, "vkCreateInstance error");
		
	}

	Queue& Device::get_general_queue() const
	{
		assert__(general_queue, "Vulkan not initialised");
		return *general_queue.get();
	}

	Queue& Device::get_transfer_queue() const
	{
		assert__(transfer_queue, "Vulkan not initialised");
		return *transfer_queue.get();
	}

	Queue& Device::get_compute_queue() const
	{
		assert__(compute_queue, "Vulkan not initialised");
		return *compute_queue.get();
	}


	Device::Device(Ref<WGI::Window> window_, bool prefer_dedicated_gpu)
		: data(make_unique<Device_>()), window(window_)
	{
		create_instance();

		assert__(glfwCreateWindowSurface(vulkan_instance, window->get_handle(), nullptr, &surface) == VK_SUCCESS, "glfwCreateWindowSurface error");

		find_physical_device(prefer_dedicated_gpu);
		create_logical_device();
		find_memory();
	}

	void Device::vulkan_wait_idle()
	{
		vkDeviceWaitIdle(device);
	}

	Device::~Device() {
		general_queue = transfer_queue = compute_queue = nullptr;

		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
		vkDestroyInstance(vulkan_instance, nullptr);
	}


	Queue& Device::get_queue(QueueType type) const
	{
		if (type == QueueType::Transfer) {
			return get_transfer_queue();
		}
		else if (type == QueueType::Compute) {
			return get_compute_queue();
		}
		return get_general_queue();
	}

	bool Device::device_usable(VkPhysicalDevice pd, bool dedicated) {
		// Extensions

		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(pd, nullptr, &extension_count, nullptr);

		vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(pd, nullptr, &extension_count, available_extensions.data());

		bool has_swapchain_extension = false;
		for (auto const& ext : available_extensions) {
			if (strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
				has_swapchain_extension = true;
				break;
			}
		}
		if (!has_swapchain_extension) {
			return false;
		}

		// Queues

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, nullptr);

		data->queue_families.resize(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, data->queue_families.data());

		bool has_graphics = false;
		bool has_transfer = false;
		bool has_present = false;
		unsigned int i = 0;
		for (auto const& q_family : data->queue_families) {
			if (q_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				has_graphics = true;

				VkBool32 present = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &present);
				if (present) {
					has_present = true;
				}
			}
			if (q_family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
				has_transfer = true;
			}

			i++;
		}
		if (!has_graphics || !has_transfer ||!has_present) {
			return false;
		}

		vkGetPhysicalDeviceProperties(pd, &data->device_properties);
		vkGetPhysicalDeviceFeatures(pd, &data->device_features);
		depth_clamp_supported = data->device_features.depthClamp;
		anisotropy_supported = data->device_features.samplerAnisotropy && data->device_properties.limits.maxSamplerAnisotropy >= 16;

		timer_multiplier = (static_cast<double>(data->device_properties.limits.timestampPeriod) / 1000.0) / 1000.0;

		return (data->device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) == dedicated;
	}

	void Device::find_physical_device(bool prefer_dedicated_gpu)
	{
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(get_vulkan_instance_handle(), &device_count, nullptr);

		assert__(device_count, "No GPUs");

		vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(get_vulkan_instance_handle(), &device_count, devices.data());

		// Find prefered graphics device type (dedicated/integrated)
		for (unsigned int i = 0; i < devices.size(); i++) {
			if (device_usable(devices[i], prefer_dedicated_gpu)) {
				physical_device = devices[i];
				break;
			}
		}

		if (!physical_device) {
			// Settle for the other option (integrated/dedicated)
			for (unsigned int i = 0; i < devices.size(); i++) {
				if (device_usable(devices[i], !prefer_dedicated_gpu)) {
					physical_device = devices[i];
					break;
				}
			}
		}

		assert__(physical_device, "No suitable GPU found");


		spdlog::info("Using device: {}", data->device_properties.deviceName);
		if (depth_clamp_supported) {
			spdlog::info("Depth clamp supported");
		}
		if(anisotropy_supported) {
			spdlog::info("Anisotropic filtering supported");
		}

	}

	void Device::create_logical_device() {
		int general_queue_family_index = -1;
		int transfer_queue_family_index = -1;
		int compute_queue_family_index = -1;

		// Find all-purpose queue

		for (unsigned int i = 0; i < data->queue_families.size(); i++) {
			if ((data->queue_families[i].queueFlags & 3) == 3) {
				VkBool32 present = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present);
				if (present) {
					general_queue_family_index = i;
					break;
				}
			}
		}

		assert__(general_queue_family_index != -1, "Could not find a graphics+present queue family");


		// Find specialised queue families (family dedicated to one of graphics/transfer/compute/present)

		for (unsigned int i = 0; i < data->queue_families.size(); i++) {
			if ((data->queue_families[i].queueFlags & 7) == VK_QUEUE_TRANSFER_BIT) {
				transfer_queue_family_index = i;
			}
			else if ((data->queue_families[i].queueFlags & 3) == VK_QUEUE_COMPUTE_BIT) {
				compute_queue_family_index = i;
			}
		}



		// Create queues

		VkDeviceQueueCreateInfo queue_create_infos[3];
		queue_create_infos[0] = {};
		queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_infos[0].queueFamilyIndex = general_queue_family_index;
		queue_create_infos[0].queueCount = 1;
		float queuePriority = 1.0f;
		queue_create_infos[0].pQueuePriorities = &queuePriority;

		int queue_create_i = 1;

		if (transfer_queue_family_index != -1) {
			queue_create_infos[1] = {};
			queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_infos[1].queueFamilyIndex = transfer_queue_family_index;
			queue_create_infos[1].queueCount = 1;
			queue_create_infos[1].pQueuePriorities = &queuePriority;
			queue_create_i++;
		}

		if (compute_queue_family_index != -1 && compute_queue_family_index != transfer_queue_family_index) {
			queue_create_infos[queue_create_i] = {};
			queue_create_infos[queue_create_i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_infos[queue_create_i].queueFamilyIndex = compute_queue_family_index;
			queue_create_infos[queue_create_i].queueCount = 1;
			queue_create_infos[queue_create_i].pQueuePriorities = &queuePriority;
			queue_create_i++;
		}

		// Create device

		VkPhysicalDeviceFeatures physical_device_features{};
		physical_device_features.depthClamp = depth_clamp_supported;
		physical_device_features.samplerAnisotropy = anisotropy_supported;

		VkDeviceCreateInfo device_create_info{};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pQueueCreateInfos = queue_create_infos;
		device_create_info.queueCreateInfoCount = queue_create_i;
		device_create_info.pEnabledFeatures = &physical_device_features;

		const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		device_create_info.enabledExtensionCount = 1;
		device_create_info.ppEnabledExtensionNames = &extension;

#ifndef NDEBUG
		device_create_info.enabledLayerCount = 1;
		const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
		device_create_info.ppEnabledLayerNames = &validation_layer_name;
#endif

		assert__(vkCreateDevice(physical_device, &device_create_info, nullptr, &device) == VK_SUCCESS, "vkCreateDevice error");

		VkQueue queue;
		vkGetDeviceQueue(device, general_queue_family_index, 0, &queue);

		general_queue = Ref<Queue>::make(device, queue, general_queue_family_index, true, true, true, 
			data->queue_families[general_queue_family_index].timestampValidBits);

		if (transfer_queue_family_index == -1) {
			transfer_queue = general_queue;
		}
		else {
			vkGetDeviceQueue(device, transfer_queue_family_index, 0, &queue);
			transfer_queue = Ref<Queue>::make(device, queue, transfer_queue_family_index, false, true,
			compute_queue_family_index == transfer_queue_family_index, 
			data->queue_families[general_queue_family_index].timestampValidBits);
		}


		if (compute_queue_family_index == -1) {
			compute_queue = general_queue;
		}
		else if (compute_queue_family_index == transfer_queue_family_index) {
			compute_queue = transfer_queue;
		}
		else {
			vkGetDeviceQueue(device, compute_queue_family_index, 0, &queue);
			compute_queue = Ref<Queue>::make(device, queue, compute_queue_family_index, false, false, true, 
			data->queue_families[compute_queue_family_index].timestampValidBits);
		}
	}
}