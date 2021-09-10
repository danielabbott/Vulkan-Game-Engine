#include <pigeon/wgi/vulkan/buffer.h>
#include <pigeon/wgi/vulkan/memory.h>
#include <pigeon/wgi/vulkan/command.h>
#include <pigeon/util.h>
#include "singleton.h"

int pigeon_vulkan_create_buffer(PigeonVulkanBuffer* buffer, uint64_t size, PigeonVulkanBufferUsages usages,
    PigeonVulkanMemoryRequirements* memory_requirements)
{
    assert(buffer && size && memory_requirements);

    VkBufferCreateInfo create = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    create.size = size;
    create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (usages.vertices) {
        create.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (usages.indices) {
        create.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (usages.transfer_dst) {
        create.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (usages.transfer_src) {
        create.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (usages.uniforms) {
        create.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (usages.ssbo) {
        create.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (usages.draw_indirect) {
        create.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }

    ASSERT__1(vkCreateBuffer(vkdev, &create, NULL, &buffer->vk_buffer) == VK_SUCCESS, "vkCreateBuffer error");

    if(singleton_data.dedicated_allocation_supported) {
		VkMemoryDedicatedRequirements dedicated_requirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};

		VkMemoryRequirements2 vk_memory_requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
		vk_memory_requirements.pNext = &dedicated_requirements;

		VkBufferMemoryRequirementsInfo2 buffer_requirements_info = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};
		buffer_requirements_info.buffer = buffer->vk_buffer;

		vkGetBufferMemoryRequirements2(vkdev, &buffer_requirements_info, &vk_memory_requirements);

		memory_requirements->alignment = (unsigned int)vk_memory_requirements.memoryRequirements.alignment;
		memory_requirements->memory_type_bits = vk_memory_requirements.memoryRequirements.memoryTypeBits;
		memory_requirements->size = vk_memory_requirements.memoryRequirements.size;
		memory_requirements->requires_dedicated = dedicated_requirements.requiresDedicatedAllocation;
		memory_requirements->prefers_dedicated = dedicated_requirements.prefersDedicatedAllocation;
		buffer->requires_dedicated_memory = dedicated_requirements.requiresDedicatedAllocation;
	}
    else {
        VkMemoryRequirements vk_memory_requirements;
        vkGetBufferMemoryRequirements(vkdev, buffer->vk_buffer, &vk_memory_requirements);

		memory_requirements->alignment = (unsigned int)vk_memory_requirements.alignment;
		memory_requirements->memory_type_bits = vk_memory_requirements.memoryTypeBits;
		memory_requirements->size = vk_memory_requirements.size;
    }

    buffer->size = size;

    return 0;
}

int pigeon_vulkan_buffer_bind_memory(PigeonVulkanBuffer* buffer, PigeonVulkanMemoryAllocation* memory, 
	uint64_t offset)
{
	ASSERT_1(buffer && buffer->vk_buffer && memory && memory->vk_device_memory && !memory->is_dedicated);
	ASSERT_1(!buffer->requires_dedicated_memory);
	ASSERT__1(vkBindBufferMemory(vkdev, buffer->vk_buffer, memory->vk_device_memory, offset) == VK_SUCCESS, "vkBindBufferMemory error");
	return 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_buffer_bind_memory_dedicated(PigeonVulkanBuffer* buffer, PigeonVulkanMemoryAllocation* memory)
{
	ASSERT_1(buffer && buffer->vk_buffer && memory && memory->vk_device_memory && memory->is_dedicated);
	ASSERT__1(vkBindBufferMemory(vkdev, buffer->vk_buffer, memory->vk_device_memory, 0) == VK_SUCCESS, "vkBindBufferMemory error");
	return 0;
}

void pigeon_vulkan_destroy_buffer(PigeonVulkanBuffer* buffer)
{
    assert(buffer);
	if (buffer && buffer->vk_buffer) {
		vkDestroyBuffer(vkdev, buffer->vk_buffer, NULL);
		buffer->vk_buffer = NULL;
	}
	else {
		assert(false);
	}
}

static int create_device_local(PigeonVulkanBuffer* buffer, PigeonVulkanMemoryAllocation * memory, 
	uint64_t size, PigeonVulkanBufferUsages usages)
{
	usages.transfer_dst = true;
	usages.transfer_src = false;

	PigeonVulkanMemoryRequirements mem_req;
	ASSERT_1(!pigeon_vulkan_create_buffer(buffer, size, usages, &mem_req));

	PigeonVulkanMemoryTypePerferences preferences = { 0 };
	preferences.device_local = PIGEON_VULKAN_MEMORY_TYPE_MUST;
	preferences.host_visible = PIGEON_VULKAN_MEMORY_TYPE_MUST_NOT;
	preferences.host_coherent = PIGEON_VULKAN_MEMORY_TYPE_MUST_NOT;
	preferences.host_cached = PIGEON_VULKAN_MEMORY_TYPE_MUST_NOT;


	if (!pigeon_vulkan_memory_type_possible(mem_req, preferences) ||
			pigeon_vulkan_allocate_memory_dedicated(memory, mem_req, preferences, NULL, buffer)) {
		pigeon_vulkan_destroy_buffer(buffer);
		return 1;
	}

	if(pigeon_vulkan_buffer_bind_memory_dedicated(buffer, memory)) {
		pigeon_vulkan_free_memory(memory);
		pigeon_vulkan_destroy_buffer(buffer);
		return 1;
	}

	return 0;
}

static int create_host_visible(PigeonVulkanBuffer* buffer, PigeonVulkanMemoryAllocation * memory, 
	uint64_t size, PigeonVulkanBufferUsages usages, bool maybe_device_local)
{
	usages.transfer_dst = false;

	PigeonVulkanMemoryRequirements mem_req;
	ASSERT_1(!pigeon_vulkan_create_buffer(buffer, size, usages, &mem_req));

	PigeonVulkanMemoryTypePerferences preferences = { 0 };
	preferences.device_local = maybe_device_local ? 
		PIGEON_VULKAN_MEMORY_TYPE_PREFERED : PIGEON_VULKAN_MEMORY_TYPE_MUST_NOT;
	preferences.host_visible = PIGEON_VULKAN_MEMORY_TYPE_MUST;
	preferences.host_coherent = PIGEON_VULKAN_MEMORY_TYPE_PREFERED;
	preferences.host_cached = PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT;

	if (pigeon_vulkan_allocate_memory_dedicated(memory, mem_req, preferences, NULL, buffer)) {
		pigeon_vulkan_destroy_buffer(buffer);
		return 1;
	}

	if(pigeon_vulkan_buffer_bind_memory_dedicated(buffer, memory)) {
		pigeon_vulkan_free_memory(memory);
		pigeon_vulkan_destroy_buffer(buffer);
		return 1;
	}
	return 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_create_staged_buffer(PigeonVulkanStagedBuffer* buffer, uint64_t size, PigeonVulkanBufferUsages usages)
{
	ASSERT_1(buffer && !buffer->buffer.vk_buffer && size);
	bool device_local_failed = create_device_local(&buffer->buffer, &buffer->memory, size, usages) != 0;

	if(device_local_failed) {
		usages.transfer_src = false;
		buffer->buffer_is_host_visible = true;
		ASSERT_1(!create_host_visible(&buffer->buffer, &buffer->memory, size, usages, true));
	}
	else {
		usages.transfer_src = true;
		buffer->buffer_is_host_visible = false;
		ASSERT_1(!create_host_visible(&buffer->staging_buffer, &buffer->staging_memory, size, usages, false));
	}

	return 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_staged_buffer_map(PigeonVulkanStagedBuffer* buffer, void ** ptr)
{
	ASSERT_1(buffer && buffer->buffer.vk_buffer && ptr);
	if(buffer->buffer_is_host_visible) {
		ASSERT_1(!pigeon_vulkan_map_memory(&buffer->memory, ptr));
	}
	else {
		ASSERT_1(!pigeon_vulkan_map_memory(&buffer->staging_memory, ptr));
	}
	return 0;
}

ERROR_RETURN_TYPE pigeon_vulkan_staged_buffer_write_done(PigeonVulkanStagedBuffer* buffer)
{
	assert(buffer && buffer->buffer.vk_buffer);

	if(buffer->buffer_is_host_visible) {
		ASSERT_1(!pigeon_vulkan_flush_memory(&buffer->memory, 0, 0));
		pigeon_vulkan_unmap_memory(&buffer->memory);
	}
	else {
		ASSERT_1(!pigeon_vulkan_flush_memory(&buffer->staging_memory, 0, 0));
		pigeon_vulkan_unmap_memory(&buffer->staging_memory);
	}
	return 0;
}

// If buffer_is_host_visible is true then don't call this.
void pigeon_vulkan_staged_buffer_transfer(PigeonVulkanStagedBuffer* buffer,
	PigeonVulkanCommandPool* pool, unsigned int command_pool_buffer_index)
{
	assert(buffer && buffer->buffer.vk_buffer && buffer->staging_buffer.vk_buffer);
	assert(!buffer->buffer_is_host_visible);

	pigeon_vulkan_buffer_transfer(pool, command_pool_buffer_index, &buffer->buffer, 0,
		&buffer->staging_buffer, 0, buffer->buffer.size);
}

void pigeon_vulkan_staged_buffer_transfer_complete(PigeonVulkanStagedBuffer* buffer)
{
	assert(buffer && buffer->buffer.vk_buffer && buffer->staging_buffer.vk_buffer);
	assert(!buffer->buffer_is_host_visible);

	pigeon_vulkan_destroy_buffer(&buffer->staging_buffer);
	pigeon_vulkan_free_memory(&buffer->staging_memory);
}

void pigeon_vulkan_destroy_staged_buffer(PigeonVulkanStagedBuffer* buffer)
{
	assert(buffer && buffer->buffer.vk_buffer);

	
	pigeon_vulkan_destroy_buffer(&buffer->buffer);
	pigeon_vulkan_free_memory(&buffer->memory);
	

	if(buffer->staging_buffer.vk_buffer) {
		pigeon_vulkan_destroy_buffer(&buffer->staging_buffer);
		pigeon_vulkan_free_memory(&buffer->staging_memory);
	}
}

ERROR_RETURN_TYPE pigeon_vulkan_create_staging_buffer_with_dedicated_memory(PigeonVulkanBuffer* buffer,
	PigeonVulkanMemoryAllocation * memory, uint64_t size, void ** mapping)
{
	ASSERT_1(buffer && memory && size && mapping);

    PigeonVulkanBufferUsages usages = {0};
    usages.transfer_src = true;
    
	PigeonVulkanMemoryRequirements mem_req;
	ASSERT_1(!pigeon_vulkan_create_buffer(buffer, size, usages, &mem_req));

	PigeonVulkanMemoryTypePerferences preferences = { 0 };
	preferences.device_local = PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT;
	preferences.host_visible = PIGEON_VULKAN_MEMORY_TYPE_MUST;
	preferences.host_coherent = PIGEON_VULKAN_MEMORY_TYPE_PREFERED;
	preferences.host_cached = PIGEON_VULKAN_MEMORY_TYPE_PREFERED_NOT;

	if (pigeon_vulkan_allocate_memory_dedicated(memory, mem_req, 
        preferences, NULL, buffer)) 
    {
		pigeon_vulkan_destroy_buffer(buffer);
		return 1;
	}

	if(pigeon_vulkan_buffer_bind_memory_dedicated(buffer, 
        memory) ||
        pigeon_vulkan_map_memory(memory, mapping)) 
    {
		pigeon_vulkan_destroy_buffer(buffer);
		pigeon_vulkan_free_memory(memory);
		return 1;
	}
	return 0;
}
