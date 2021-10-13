#include <pigeon/wgi/vulkan/semaphore.h>
#include "singleton.h"
#include <pigeon/assert.h>

PIGEON_ERR_RET pigeon_vulkan_create_semaphore(PigeonVulkanSemaphore* semaphore)
{
	assert(semaphore);
	
    VkSemaphoreCreateInfo semaphore_create = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	ASSERT_LOG_R1(vkCreateSemaphore(vkdev, &semaphore_create, NULL, &semaphore->vk_semaphore) == VK_SUCCESS,
		"vkCreateSemaphore error");
	return 0;
}

void pigeon_vulkan_destroy_semaphore(PigeonVulkanSemaphore* semaphore)
{
	if (semaphore && semaphore->vk_semaphore) {
		vkDestroySemaphore(vkdev, semaphore->vk_semaphore, NULL);
		semaphore->vk_semaphore = NULL;
	}
	else {
		assert(false);
	}
}
