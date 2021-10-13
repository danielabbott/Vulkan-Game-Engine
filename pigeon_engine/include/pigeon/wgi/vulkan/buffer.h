#pragma once

#include <stdbool.h>
#include "memory.h"

struct VkBuffer_T;
struct PigeonVulkanCommandPool;

typedef struct PigeonVulkanBufferUsages {
	bool vertices;
	bool indices;
	bool uniforms;
	bool transfer_dst;
	bool transfer_src;
	bool ssbo;
	bool draw_indirect;
} PigeonVulkanBufferUsages;

typedef struct PigeonVulkanBuffer {
	struct VkBuffer_T* vk_buffer;
	uint64_t size;
	bool requires_dedicated_memory;
} PigeonVulkanBuffer;

PIGEON_ERR_RET pigeon_vulkan_create_buffer(PigeonVulkanBuffer*, uint64_t size, PigeonVulkanBufferUsages, PigeonVulkanMemoryRequirements*);

PIGEON_ERR_RET pigeon_vulkan_buffer_bind_memory(PigeonVulkanBuffer*, struct PigeonVulkanMemoryAllocation*, uint64_t offset);
PIGEON_ERR_RET pigeon_vulkan_buffer_bind_memory_dedicated(PigeonVulkanBuffer*, struct PigeonVulkanMemoryAllocation*);
void pigeon_vulkan_destroy_buffer(PigeonVulkanBuffer*);


// Creates device-local and host-visible buffers (dedicated memory allocation for each)
// Then transfers the contents from staging buffer to device-local and deletes staging buffer
typedef struct PigeonVulkanStagedBuffer {
	PigeonVulkanBuffer buffer;
	struct PigeonVulkanMemoryAllocation memory;

	// true if couldn't create a device-local buffer so only one host-visible buffer is used
	bool buffer_is_host_visible;

	PigeonVulkanBuffer staging_buffer;
	struct PigeonVulkanMemoryAllocation staging_memory;
} PigeonVulkanStagedBuffer;

PIGEON_ERR_RET pigeon_vulkan_create_staged_buffer(PigeonVulkanStagedBuffer*, uint64_t size, PigeonVulkanBufferUsages);
PIGEON_ERR_RET pigeon_vulkan_staged_buffer_map(PigeonVulkanStagedBuffer*, void **);
PIGEON_ERR_RET pigeon_vulkan_staged_buffer_write_done(PigeonVulkanStagedBuffer*);

// If buffer_is_host_visible is true then don't call these 2 functions

void pigeon_vulkan_staged_buffer_transfer(PigeonVulkanStagedBuffer*,
	struct PigeonVulkanCommandPool*, unsigned int command_pool_buffer_index);
void pigeon_vulkan_staged_buffer_transfer_complete(PigeonVulkanStagedBuffer*);

void pigeon_vulkan_destroy_staged_buffer(PigeonVulkanStagedBuffer*);

// Convenience function
PIGEON_ERR_RET pigeon_vulkan_create_staging_buffer_with_dedicated_memory(PigeonVulkanBuffer* buffer,
	PigeonVulkanMemoryAllocation * memory, uint64_t size, void ** mapping);
