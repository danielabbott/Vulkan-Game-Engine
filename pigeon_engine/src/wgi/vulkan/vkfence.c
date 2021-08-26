#include <pigeon/wgi/vulkan/fence.h>
#include "singleton.h"
#include <pigeon/util.h>

ERROR_RETURN_TYPE pigeon_vulkan_create_fence(PigeonVulkanFence* fence, bool start_triggered)
{
	VkFenceCreateInfo fence_create = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fence_create.flags = start_triggered ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	ASSERT__1(vkCreateFence(vkdev, &fence_create, NULL, &fence->vk_fence) == VK_SUCCESS,
		"vkCreateFence error");
	return 0;
}


ERROR_RETURN_TYPE pigeon_vulkan_wait_fence(PigeonVulkanFence* fence)
{
	ASSERT__1(vkWaitForFences(vkdev, 1, &fence->vk_fence, VK_TRUE, 3000000000) == VK_SUCCESS, "vkWaitForFences error");
	return 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_poll_fence(PigeonVulkanFence* fence, bool * state)
{
	VkResult status = vkGetFenceStatus(vkdev, fence->vk_fence);
	ASSERT__1(status == VK_SUCCESS || status == VK_NOT_READY, "vkGetFenceStatus error");
	*state = status == VK_SUCCESS;
	return 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_reset_fence(PigeonVulkanFence* fence)
{
	assert(fence);

	ASSERT__1(vkResetFences(vkdev, 1, &fence->vk_fence) == VK_SUCCESS, "vkResetFences error");
	return 0;
}

void pigeon_vulkan_destroy_fence(PigeonVulkanFence* fence)
{
	if (fence && fence->vk_fence) {
		vkDestroyFence(vkdev, fence->vk_fence, NULL);
		fence->vk_fence = NULL;
	}
	else {
		assert(false);
	}
}
