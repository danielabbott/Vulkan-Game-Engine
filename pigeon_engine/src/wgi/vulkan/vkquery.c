#include <pigeon/wgi/vulkan/query.h>
#include "singleton.h"

bool pigeon_vulkan_general_queue_supports_timestamps(void)
{
    return singleton_data.general_queue_timestamp_bits_valid != 0;
}
bool pigeon_vulkan_transfer_queue_supports_timestamps(void)
{
    return singleton_data.transfer_queue_timestamp_bits_valid != 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_create_timer_query_pool(PigeonVulkanTimerQueryPool* pool, unsigned int num_query_objects)
{
    VkQueryPoolCreateInfo create_info = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };

    create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    if(num_query_objects > 32) num_query_objects = 32;
    create_info.queryCount = num_query_objects;

    ASSERT__1(vkCreateQueryPool(vkdev, &create_info, NULL, &pool->vk_query_pool) == VK_SUCCESS,
        "vkCreateQueryPool error");

    pool->num_query_objects = num_query_objects;
    return 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_get_timer_results(PigeonVulkanTimerQueryPool* pool, double* output)
{
    if(pool->num_query_objects > 32) pool->num_query_objects = 32;
    uint64_t int_data[32];

    ASSERT__1(vkGetQueryPoolResults(vkdev, pool->vk_query_pool, 0, pool->num_query_objects,
        pool->num_query_objects*8, int_data, 8, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) == VK_SUCCESS,
        "vkGetQueryPoolResults error");

    for(unsigned int i = 0; i < pool->num_query_objects; i++) {
        output[i] = (double)int_data[i] * singleton_data.timer_multiplier;
    }
    return 0;
}

void pigeon_vulkan_destroy_timer_query_pool(PigeonVulkanTimerQueryPool* pool)
{
    if (pool && pool->vk_query_pool) {
		vkDestroyQueryPool(vkdev, pool->vk_query_pool, NULL);
		pool->vk_query_pool = NULL;
	}
	else {
		assert(false);
	}
}
