#pragma once

#include "../Ref.h"
#include "Queue.h"
#include "Memory.h"
#include <memory>
#include <vector>

namespace WGI {
	class Window;
}

struct VkInstance_T;
struct VkPhysicalDevice_T;
struct VkSurfaceKHR_T;
struct VkPhysicalDeviceMemoryProperties;

namespace Vulkan {

	struct Device_;
	class Queue;


	class Device {
		std::unique_ptr<Device_> data;

		Ref<WGI::Window> window;
		VkPhysicalDevice_T* physical_device = nullptr;
		VkDevice_T* device = nullptr;
		VkSurfaceKHR_T* surface = nullptr;
		double timer_multiplier = 0;

		Ref<Queue> general_queue;
		Ref<Queue> transfer_queue;
		Ref<Queue> compute_queue;

		bool depth_clamp_supported, anisotropy_supported;

		void create_instance();

		bool device_usable(VkPhysicalDevice_T* pd, bool dedicated);
		void find_physical_device(bool prefer_dedicated_gpu);
		void create_logical_device();
		void find_memory();
	public:
		// TODO should Window be a reference type?
		Device(Ref<WGI::Window>, bool prefer_dedicated_gpu);
		~Device();
		void vulkan_wait_idle();

		Ref<WGI::Window> const& get_window() const;

		bool supports_depth_clamp() const;
		bool supports_anisotropic_filtering() const;


		Queue& get_general_queue() const;
		Queue& get_transfer_queue() const;
		Queue& get_compute_queue() const;
		Queue& get_queue(QueueType) const;

		// Multiplier to convert timestamp integer values into milliseconds
		double get_timer_multiplier() const { return timer_multiplier; }

		VkPhysicalDeviceMemoryProperties const& get_memory_properties() const;
		VkPhysicalDeviceMemoryProperties & get_memory_properties_();

		// Returns true if any memory type matches the criteria
		bool vulkan_memory_type_exists(WGI::MemoryCriteria);

		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;
		Device(Device&&) = delete;
		Device& operator=(Device&&) = delete;

		VkDevice_T* get_device_handle_() const { return device; }
		VkPhysicalDevice_T* get_physical_device_handle_() const { return physical_device; }
		VkSurfaceKHR_T* get_surface_handle_() const { return surface; }
	};

	VkInstance_T* get_vulkan_instance_handle();

}