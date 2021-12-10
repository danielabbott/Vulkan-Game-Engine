#include "Device.h"
#include "Include.h"

using namespace std;

namespace Vulkan {

	void Device::find_memory()
	{
		vkGetPhysicalDeviceMemoryProperties(physical_device, &get_memory_properties_());
	}

	MemoryAllocation::MemoryAllocation(Device& device_, WGI::MemoryCriteria criteria)
		: device(device_), size(criteria.size)
	{
		auto const& memory_properties = device.get_memory_properties();

		vector<int> memory_type_scores(memory_properties.memoryTypeCount, 0);

		// Calculate scores.

		for (unsigned int i = 0; i < memory_type_scores.size(); i++) {
			auto& score = memory_type_scores[i];
			VkMemoryPropertyFlags flags = memory_properties.memoryTypes[i].propertyFlags;

			if (!(criteria.memory_type_bits & (1 << i))
				|| (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
				|| (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)) {
				score = -99999;
				continue;
			}


			const auto check_flag = [&score, flags](unsigned int vk_flag, WGI::MemoryTypePerference preference) {
				if (flags & vk_flag) {
					if (preference == WGI::MemoryTypePerference::Must
						|| preference == WGI::MemoryTypePerference::Prefered) {
						score++;
					}
					else if (preference == WGI::MemoryTypePerference::PreferedNot) {
						score--;
					}
					else if (preference == WGI::MemoryTypePerference::MustNot) {
						score = -99999;
					}
				}
				else {
					if (preference == WGI::MemoryTypePerference::Must) {
						score = -99999;
					}
					else if (preference == WGI::MemoryTypePerference::Prefered) {
						score--;
					}
					else if (preference == WGI::MemoryTypePerference::MustNot
						|| preference == WGI::MemoryTypePerference::PreferedNot) {
						score++;
					}
				}
			};
			check_flag(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, criteria.device_local);
			check_flag(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, criteria.host_visible);
			check_flag(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, criteria.host_coherent);

		}

		int max_value = -99999;
		unsigned int max_index = 0;

		for (unsigned int i = 0; i < memory_type_scores.size(); i++) {
			if (memory_type_scores[i] > max_value) {
				max_value = memory_type_scores[i];
				max_index = i;
			}
		}

		if (max_value < -100) {
			throw runtime_error("No valid Vulkan memory types");
		}

		memory_type_index = max_index;
		heap_index = memory_properties.memoryTypes[memory_type_index].heapIndex;

		VkMemoryPropertyFlags flags = memory_properties.memoryTypes[memory_type_index].propertyFlags;
		properties.device_local = (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0;
		properties.host_coherent = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
		properties.host_visible = (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

		VkMemoryAllocateInfo alloc{};
		alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc.allocationSize = criteria.size;
		alloc.memoryTypeIndex = memory_type_index;

		assert__(vkAllocateMemory(device.get_device_handle_(), &alloc, nullptr, &memory_handle) == VK_SUCCESS, "vkAllocateMemory error");


	}

	bool Device::vulkan_memory_type_exists(WGI::MemoryCriteria criteria)
	{
		auto const& memory_properties = get_memory_properties();


		for (unsigned int i = 0; i < memory_properties.memoryTypeCount; i++) {
			VkMemoryPropertyFlags flags = memory_properties.memoryTypes[i].propertyFlags;

			if (criteria.device_local == WGI::MemoryTypePerference::Must
				&& !(flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				continue;
			}
			if (criteria.device_local == WGI::MemoryTypePerference::MustNot
				&& (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				continue;
			}
			if (criteria.host_visible == WGI::MemoryTypePerference::Must
				&& !(flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
				continue;
			}
			if (criteria.host_visible == WGI::MemoryTypePerference::MustNot
				&& (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
				continue;
			}
			if (criteria.host_coherent == WGI::MemoryTypePerference::Must
				&& !(flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				continue;
			}
			if (criteria.host_coherent == WGI::MemoryTypePerference::MustNot
				&& (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				continue;
			}
			if (criteria.host_cached == WGI::MemoryTypePerference::Must
				&& !(flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
				continue;
			}
			if (criteria.host_cached == WGI::MemoryTypePerference::MustNot
				&& (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
				continue;
			}

			return true;
		}
		return false;
	}

	MemoryAllocation::~MemoryAllocation()
	{
		unmap();
		vkFreeMemory(device.get_device_handle_(), memory_handle, nullptr);
	}

	ArrayPointer<uint8_t> MemoryAllocation::map()
	{
		if (!memory_pointer.has_value()) {
			void* p;
			assert__(vkMapMemory(device.get_device_handle_(), memory_handle, 0, VK_WHOLE_SIZE, 0, &p) == VK_SUCCESS, "vkMapMemory error");
			memory_pointer = p;
		}
		return ArrayPointer<uint8_t>(reinterpret_cast<uint8_t*>(memory_pointer.value()), size);
	}

	void MemoryAllocation::unmap()
	{
		if (memory_pointer.has_value()) {
			vkUnmapMemory(device.get_device_handle_(), memory_handle);
			memory_pointer = nullopt;
		}
	}


	// Unmaps if persistent mappings are not optimal for this memory type
	void MemoryAllocation::maybe_unmap()
	{
		auto const& memory_properties = device.get_memory_properties();
		auto flags = memory_properties.memoryTypes[memory_type_index].propertyFlags;

		if (!(flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			unmap();
		}
	}



	// After write
	void MemoryAllocation::flush_region(WGI::MemoryRegion region)
	{
		auto const& memory_properties = device.get_memory_properties();
		auto flags = memory_properties.memoryTypes[memory_type_index].propertyFlags;

		if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
			return;
		}

		VkMappedMemoryRange range{};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = memory_handle;
		range.offset = region.start;
		range.size = region.start;

		assert__(vkFlushMappedMemoryRanges(device.get_device_handle_(), 1, &range) == VK_SUCCESS, "vkFlushMappedMemoryRanges error");
	}

	// Before read
	void MemoryAllocation::invalidate_region(WGI::MemoryRegion region)
	{
		auto const& memory_properties = device.get_memory_properties();
		auto flags = memory_properties.memoryTypes[memory_type_index].propertyFlags;

		if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
			return;
		}

		VkMappedMemoryRange range{};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = memory_handle;
		range.offset = region.start;
		range.size = region.start;

		assert__(vkInvalidateMappedMemoryRanges(device.get_device_handle_(), 1, &range) == VK_SUCCESS, "vkInvalidateMappedMemoryRanges error");

	}

}