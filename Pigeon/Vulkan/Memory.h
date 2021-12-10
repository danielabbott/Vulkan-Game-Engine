#pragma once

#include "../Ref.h"
#include <optional>
#include "../ArrayPointer.h"
#include "../WGI/Memory.h"


struct VkDeviceMemory_T;

namespace Vulkan {

	class Device;

	

	struct VulkanMemoryProperties {
		bool device_local = false;
		bool host_visible = false;
		bool host_coherent = false;
	};

	class MemoryAllocation {
		Device& device;
		VkDeviceMemory_T* memory_handle;

		VulkanMemoryProperties properties;

		unsigned int heap_index;
		unsigned int memory_type_index;


		unsigned int size;

		std::optional<void*> memory_pointer = std::nullopt;

	public:

		MemoryAllocation(Device&, WGI::MemoryCriteria);
		~MemoryAllocation();

		unsigned int get_size() const { return size; }
		VulkanMemoryProperties get_properties() const { return properties; }
		VkDeviceMemory_T* get_handle() const { return memory_handle; }

		// Mapping pointer is cached so map() can be called repeatedly
		ArrayPointer<uint8_t> map();
		void unmap();

		// Unmaps if persistent mappings are not optimal for this memory type
		void maybe_unmap();

		// If the memory has the coherent bit set then these will do nothing

		// After write
		void flush_region(WGI::MemoryRegion region);
		void flush() { flush_region(WGI::MemoryRegion(0, size)); }

		// Before read
		void invalidate_region(WGI::MemoryRegion region);
		void invalidate() { invalidate_region(WGI::MemoryRegion(0, size)); }

	};

	class StackAllocator {
		Ref<MemoryAllocation> memory;
		unsigned int allocations = 0;
		unsigned int memory_allocated = 0;

		// Region within MemoryAllocation to allocate from
		unsigned int offset;
		unsigned int size;

	public:
		StackAllocator(Ref<MemoryAllocation>, unsigned int offset, unsigned int size);

		// Allocated region may be bigger than requested in order to meet alignment constraints
		WGI::MemoryRegion allocate(unsigned int size, unsigned int alignment);
		std::optional<WGI::MemoryRegion> try_allocate(unsigned int size, unsigned int alignment);

		// region should be the last MemoryRegion to be returned by allocate or try_allocate
		void free_last(WGI::MemoryRegion region);
	};
}