#include <pigeon/wgi/vulkan/memory.h>
#include "singleton.h"
#include <assert.h>
#include <stdlib.h>
#include <pigeon/util.h>
#include <pigeon/wgi/vulkan/image.h>
#include <pigeon/wgi/vulkan/buffer.h>

static void check_flag(int * score, VkMemoryPropertyFlags flags, 
	unsigned int vk_flag, PigeonVulkanMemoryTypePerference preference)
{
	if (flags & vk_flag) {
		if (preference == PIGEON_VULKAN_MEMORY_TYPE_MUST
			|| preference == PIGEON_VULKAN_MEMORY_TYPE_PREFERRED) {
			(*score)++;
		}
		else if (preference == PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT) {
			(*score)--;
		}
		else if (preference == PIGEON_VULKAN_MEMORY_TYPE_MUST_NOT) {
			*score = -99999;
		}
	}
	else {
		if (preference == PIGEON_VULKAN_MEMORY_TYPE_MUST) {
			*score = -99999;
		}
		else if (preference == PIGEON_VULKAN_MEMORY_TYPE_PREFERRED) {
			(*score)--;
		}
		else if (preference == PIGEON_VULKAN_MEMORY_TYPE_MUST_NOT
			|| preference == PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT) {
			(*score)++;
		}
	}
}

static int get_best_memory_type(uint32_t * best_memory_type_index,
	PigeonVulkanMemoryRequirements memory_req, PigeonVulkanMemoryTypePreferences preferences)
{
	int* memory_type_scores = malloc(singleton_data.memory_properties.memoryTypeCount * sizeof *memory_type_scores);
	ASSERT_1(memory_type_scores);

	for (unsigned int i = 0; i < singleton_data.memory_properties.memoryTypeCount; i++) {
		VkMemoryPropertyFlags flags = singleton_data.memory_properties.memoryTypes[i].propertyFlags;

		if (!(memory_req.memory_type_bits & (1 << i))
			|| (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
			|| (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)) {
			memory_type_scores[i] = -99999;
			continue;
		}

		memory_type_scores[i] = 0;
		check_flag(&memory_type_scores[i], flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, preferences.device_local);
		check_flag(&memory_type_scores[i], flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, preferences.host_visible);
		check_flag(&memory_type_scores[i], flags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, preferences.host_coherent);
		check_flag(&memory_type_scores[i], flags, VK_MEMORY_PROPERTY_HOST_CACHED_BIT, preferences.host_cached);

	}

	int max_value = -99999;
	unsigned int max_index = 0;

	for (unsigned int i = 0; i < singleton_data.memory_properties.memoryTypeCount; i++) {
		if (memory_type_scores[i] > max_value) {
			max_value = memory_type_scores[i];
			max_index = i;
		}
	}
	*best_memory_type_index = max_index;
	free(memory_type_scores);

	if (max_value < -100) {
		return 1;
	}


	return 0;
}

bool pigeon_vulkan_memory_type_possible(PigeonVulkanMemoryRequirements memory_req, PigeonVulkanMemoryTypePreferences preferences)
{
	unsigned int best;
	return get_best_memory_type(&best, memory_req, preferences) == 0;
}

static int do_allocatate(PigeonVulkanMemoryAllocation* memory, PigeonVulkanMemoryRequirements memory_req,	
	PigeonVulkanMemoryTypePreferences preferences, bool dedicated, 
	PigeonVulkanImage* image, PigeonVulkanBuffer * buffer)
{
	ASSERT_1(memory);
	if(dedicated) ASSERT_1((image || buffer) && (!image || !buffer) && singleton_data.dedicated_allocation_supported);

	uint32_t best_memory_type_index;
	ASSERT_1(!get_best_memory_type(&best_memory_type_index, memory_req, preferences));

	VkMemoryPropertyFlags flags = singleton_data.memory_properties.memoryTypes[best_memory_type_index].propertyFlags;

	memory->device_local = (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0;
	memory->host_coherent = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
	memory->host_visible = (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
	memory->mapping = NULL;
	memory->is_dedicated = dedicated;

	VkMemoryAllocateInfo alloc = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	alloc.allocationSize = memory_req.size;
	alloc.memoryTypeIndex = best_memory_type_index;

	VkMemoryDedicatedAllocateInfoKHR dedicated_info = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR};
	if(dedicated) {
		if(image) dedicated_info.image = image->vk_image;
		if(buffer) dedicated_info.buffer = buffer->vk_buffer;
		alloc.pNext = &dedicated_info;
	}

	ASSERT__1(vkAllocateMemory(vkdev, &alloc, NULL, &memory->vk_device_memory) == VK_SUCCESS, "vkAllocateMemory error");
	return 0;
}

int pigeon_vulkan_allocate_memory(PigeonVulkanMemoryAllocation* memory, 
	PigeonVulkanMemoryRequirements memory_req,	PigeonVulkanMemoryTypePreferences preferences)
{
	return do_allocatate(memory, memory_req, preferences, false, NULL, NULL);
}

int pigeon_vulkan_allocate_memory_dedicated(PigeonVulkanMemoryAllocation* memory, 
	PigeonVulkanMemoryRequirements memory_req,	PigeonVulkanMemoryTypePreferences preferences, 
	PigeonVulkanImage* image, PigeonVulkanBuffer * buffer)
{
	return do_allocatate(memory, memory_req, preferences, singleton_data.dedicated_allocation_supported, image, buffer);
}

void pigeon_vulkan_free_memory(PigeonVulkanMemoryAllocation* memory)
{
	if (memory && memory->vk_device_memory) {
		vkFreeMemory(vkdev, memory->vk_device_memory, NULL);
		memory->vk_device_memory = NULL;
	}
	else {
		assert(false);
	}
}


ERROR_RETURN_TYPE pigeon_vulkan_map_memory(PigeonVulkanMemoryAllocation* memory, void** data_ptr)
{
	assert(memory);
	if (!memory->mapping) {
		ASSERT__1(vkMapMemory(vkdev, memory->vk_device_memory, 0, VK_WHOLE_SIZE, 0, &memory->mapping)
			== VK_SUCCESS, "vkMapMemory error");

		if(data_ptr) *data_ptr = memory->mapping;
	}
	return 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_flush_memory(PigeonVulkanMemoryAllocation* memory, uint64_t offset, uint64_t size)
{
	if (!size) return 0;
	assert(memory);
	
	if (!memory->host_coherent) {
		VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		range.memory = memory->vk_device_memory;
		range.offset = offset;
		range.size = size ? size : VK_WHOLE_SIZE;

		ASSERT__1(vkFlushMappedMemoryRanges(vkdev, 1, &range) == VK_SUCCESS, "vkFlushMappedMemoryRanges error");
	}
	return 0;
}

void pigeon_vulkan_unmap_memory(PigeonVulkanMemoryAllocation* memory)
{
	if (memory->mapping) {
		vkUnmapMemory(vkdev, memory->vk_device_memory);
		memory->mapping = NULL;
	}
}
