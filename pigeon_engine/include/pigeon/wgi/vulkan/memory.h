#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <pigeon/util.h>


struct VkDeviceMemory_T;
struct PigeonVulkanBuffer;
struct PigeonVulkanImage;

typedef enum {
	PIGEON_VULKAN_MEMORY_TYPE_DONT_CARE,
	PIGEON_VULKAN_MEMORY_TYPE_MUST_NOT,
	PIGEON_VULKAN_MEMORY_TYPE_PREFERRED_NOT,
	PIGEON_VULKAN_MEMORY_TYPE_PREFERRED,
	PIGEON_VULKAN_MEMORY_TYPE_MUST,
} PigeonVulkanMemoryTypePerference;

typedef struct PigeonVulkanMemoryTypePreferences {
	PigeonVulkanMemoryTypePerference device_local;
	PigeonVulkanMemoryTypePerference host_visible;
	PigeonVulkanMemoryTypePerference host_coherent;

	// Non-cached memory is best for sequential writes on CPU and reads on GPU
	// Cached memory is best for writes on GPU and reads on CPU
	PigeonVulkanMemoryTypePerference host_cached;
} PigeonVulkanMemoryTypePreferences;

typedef struct PigeonVulkanMemoryRequirements
{
	unsigned int alignment;
	unsigned int memory_type_bits;
	uint64_t size;
	bool requires_dedicated;
	bool prefers_dedicated;
} PigeonVulkanMemoryRequirements;

typedef struct PigeonVulkanMemoryAllocation
{
	struct VkDeviceMemory_T* vk_device_memory;
	bool device_local;
	bool host_visible;
	bool host_coherent;
	void* mapping;
	bool is_dedicated;
} PigeonVulkanMemoryAllocation;

bool pigeon_vulkan_memory_type_possible(PigeonVulkanMemoryRequirements memory_req, PigeonVulkanMemoryTypePreferences preferences);

int pigeon_vulkan_allocate_memory(PigeonVulkanMemoryAllocation*, 
	PigeonVulkanMemoryRequirements, PigeonVulkanMemoryTypePreferences);
int pigeon_vulkan_allocate_memory_dedicated(PigeonVulkanMemoryAllocation*, 
	PigeonVulkanMemoryRequirements, PigeonVulkanMemoryTypePreferences,
	// One must be valid, the other NULL
	struct PigeonVulkanImage* image, struct PigeonVulkanBuffer * buffer);


ERROR_RETURN_TYPE pigeon_vulkan_map_memory(PigeonVulkanMemoryAllocation*, void** data_ptr);
ERROR_RETURN_TYPE pigeon_vulkan_flush_memory(PigeonVulkanMemoryAllocation*, uint64_t offset, uint64_t size);
void pigeon_vulkan_unmap_memory(PigeonVulkanMemoryAllocation*);


void pigeon_vulkan_free_memory(PigeonVulkanMemoryAllocation*);
