#pragma once

#include <stdbool.h>
#include <pigeon/util.h>

struct VkSemaphore_T;

typedef struct PigeonVulkanSemaphore {
	struct VkSemaphore_T* vk_semaphore;
} PigeonVulkanSemaphore;


ERROR_RETURN_TYPE pigeon_vulkan_create_semaphore(PigeonVulkanSemaphore*);

ERROR_RETURN_TYPE pigeon_vulkan_reset_semaphore(PigeonVulkanSemaphore*);

void pigeon_vulkan_destroy_semaphore(PigeonVulkanSemaphore*);
