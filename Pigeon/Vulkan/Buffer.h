#pragma once

#include "Device.h"
#include "Memory.h"

#include <memory>
#include "../Ref.h"


struct VkBuffer_T;

namespace Vulkan {

	struct BufferUsages {
		bool vertices = false;
		bool indices = false;
		bool uniforms = false;
		bool transfer_dst = false;
		bool transfer_src = false;
		bool texture_sample = false;
	};

	class Buffer {
		Device& device;
		VkBuffer_T* buffer = nullptr;
		uint32_t size;

		std::optional<Ref<MemoryAllocation>> memory = std::nullopt;
		VulkanMemoryProperties memory_properties; // valid if memory is valid
		WGI::MemoryRegion memory_region;
		BufferUsages usages;
	public:
		// MemoryCriteria::memory_type_bits is not used
		Buffer(Device&, uint32_t bytes, WGI::MemoryCriteria&, BufferUsages usages);

		void bind_memory(Ref<MemoryAllocation>, WGI::MemoryRegion);
		bool has_memory() const { return memory.has_value(); }

		uint32_t get_size() const { return size; }
		VkBuffer_T* get_handle() const { return buffer; }
		VulkanMemoryProperties get_memory_properties() const { return memory_properties; }
		BufferUsages get_usages() const { return usages; }

		MemoryAllocation& get_memory() const { return *memory.value(); }
		WGI::MemoryRegion get_memory_region() const { return memory_region; }

		~Buffer();


		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;
		Buffer(Buffer&& b) = delete;
		Buffer& operator=(Buffer&& b) = delete;


		//private:
		//	void do_move(Buffer&& b) noexcept {
		//		device = move(b.device);
		//		buffer = b.buffer;
		//		b.buffer = nullptr;
		//		size = b.size;
		//		memory = move(b.memory);
		//		memory_properties = b.memory_properties;
		//		memory_region = b.memory_region;
		//		memory_pointer = b.memory_pointer;
		//		usages = b.usages;
		//	}
		//public:
		//
		//	Buffer(Buffer&& b) noexcept {
		//		do_move(move(b));
		//	}
		//	Buffer& operator=(Buffer&& b) noexcept {
		//		do_move(move(b));
		//		return *this;
		//	}
	};

}