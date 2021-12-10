#pragma once

// Don't create queue objects, use the functions in Device to get a reference to a queue


#include <cstdint>

struct VkDevice_T;
struct VkPhysicalDevice_T;
struct VkQueue_T;
struct VkCommandPool_T;
struct VkCommandBuffer_T;

namespace Vulkan {

	class RenderPass;
	class Framebuffer;
	class Pipeline;
	class Swapchain;

	enum class QueueType {
		Graphics, Transfer, Compute
	};

	class Queue {
		friend class Device;

		VkDevice_T* device;
		VkQueue_T* queue_handle = nullptr;

		uint32_t family_index;

		bool graphics = false, transfer = false, compute = false;

		unsigned int timestamp_bits_valid = 0;


	public:

		Queue(VkDevice_T*, VkQueue_T*, uint32_t family_index, bool graphics_, bool transfer_, bool compute_, unsigned int timestamp_bits_valid_);
		~Queue();


		uint32_t get_family_index() const { return family_index; }
		bool supports_graphics() { return graphics; }
		bool supports_transfer() { return transfer; }

		VkQueue_T* get_queue_handle() const { return queue_handle; }

		// 0 = timestamp not supported
		unsigned int get_timestamp_bits_valid() const { return timestamp_bits_valid; }



		Queue(const Queue&) = delete;
		Queue& operator=(const Queue&) = delete;
		//Queue(Queue&&) noexcept;
		//Queue& operator=(Queue&&) noexcept;
		Queue(Queue&&) = delete;
		Queue& operator=(Queue&&) = delete;
	};

}