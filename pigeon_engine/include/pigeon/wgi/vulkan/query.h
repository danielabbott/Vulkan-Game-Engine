#pragma once

#include <pigeon/util.h>
#include <stdbool.h>

struct VkQueryPool_T;

bool pigeon_vulkan_general_queue_supports_timestamps(void);
bool pigeon_vulkan_transfer_queue_supports_timestamps(void);


// typedef enum {
// 	PIGEON_VULKAN_QUERY_TYPE_TIMER
// } PigeonVulkanQueryType;

typedef struct PigeonVulkanTimerQueryPool {
	struct VkQueryPool_T* vk_query_pool;
    unsigned int num_query_objects;
} PigeonVulkanTimerQueryPool;

PIGEON_ERR_RET pigeon_vulkan_create_timer_query_pool(PigeonVulkanTimerQueryPool*, unsigned int num_query_objects);
PIGEON_ERR_RET pigeon_vulkan_get_timer_results(PigeonVulkanTimerQueryPool*, double*);
void pigeon_vulkan_destroy_timer_query_pool(PigeonVulkanTimerQueryPool*);
