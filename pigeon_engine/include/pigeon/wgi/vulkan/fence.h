#pragma once

#include <stdbool.h>
#include <pigeon/util.h>

struct VkFence_T;

typedef struct PigeonVulkanFence {
	struct VkFence_T* vk_fence;
} PigeonVulkanFence;


PIGEON_ERR_RET pigeon_vulkan_create_fence(PigeonVulkanFence*, bool start_triggered);

// Timeout 3 seconds
PIGEON_ERR_RET pigeon_vulkan_wait_fence(PigeonVulkanFence*);

PIGEON_ERR_RET pigeon_vulkan_poll_fence(PigeonVulkanFence* fence, bool* state);

PIGEON_ERR_RET pigeon_vulkan_reset_fence(PigeonVulkanFence*);

void pigeon_vulkan_destroy_fence(PigeonVulkanFence*);
