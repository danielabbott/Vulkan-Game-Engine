#include "singleton.h"
#include <pigeon/wgi/vulkan/descriptor.h>
#include <pigeon/util.h>
#include <stdlib.h>

int pigeon_vulkan_create_descriptor_layout(PigeonVulkanDescriptorLayout * layout,
	unsigned int bindings_count, PigeonVulkanDescriptorBinding* bindings)
{
	if (!layout || !bindings_count || !bindings) {
		assert(false);
		return 1;
	}

	VkDescriptorSetLayoutBinding* layouts = calloc(bindings_count, sizeof *layouts);
	if (!layouts) return 1;

	layout->number_of_textures = 0;
	layout->number_of_storage_images = 0;
	layout->number_of_uniforms = 0;
	layout->number_of_ssbos = 0;

	for (unsigned int i = 0; i < bindings_count; i++) {
		if (bindings[i].elements == 0) {
			bindings[i].elements = 1;
		}

		switch (bindings[i].type) {
		case PIGEON_VULKAN_DESCRIPTOR_TYPE_UNIFORM:
			layout->number_of_uniforms += bindings[i].elements;
			layouts[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			break;
		case PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE:
			layout->number_of_textures += bindings[i].elements;
			layouts[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			break;
		case PIGEON_VULKAN_DESCRIPTOR_TYPE_IMAGE:
			layout->number_of_storage_images += bindings[i].elements;
			layouts[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			break;
		case PIGEON_VULKAN_DESCRIPTOR_TYPE_SSBO:
			layout->number_of_ssbos += bindings[i].elements;
			layouts[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			break;
		}


		layouts[i].descriptorCount = bindings[i].elements;
		layouts[i].stageFlags = 0;
		if (bindings[i].vertex_shader_accessible) {
			layouts[i].stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		if (bindings[i].fragment_shader_accessible) {
			layouts[i].stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		if (bindings[i].compute_shader_accessible) {
			layouts[i].stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
		}
		
		layouts[i].binding = i;
	}

	VkDescriptorSetLayoutCreateInfo layout_create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layout_create_info.bindingCount = bindings_count;
	layout_create_info.pBindings = layouts;

	ASSERT__1(vkCreateDescriptorSetLayout(vkdev, &layout_create_info, NULL, &layout->handle) == VK_SUCCESS,
		"vkCreateDescriptorSetLayout error");

	free(layouts);
	return 0;
}

void pigeon_vulkan_destroy_descriptor_layout(PigeonVulkanDescriptorLayout* layout)
{
	if (layout && layout->handle) {
		vkDestroyDescriptorSetLayout(vkdev, layout->handle, NULL);
		layout->handle = NULL;
	}
	else {
		assert(false);
	}
}

static int create_pool(PigeonVulkanDescriptorPool* pool, 
	unsigned int set_count, PigeonVulkanDescriptorLayout* layouts)
{
	unsigned int uniform_descriptors_total = 0;
	unsigned int texture_descriptors_total = 0;
	unsigned int storage_image_descriptors_total = 0;
	unsigned int ssbo_descriptors_total = 0;
	for(unsigned int i = 0; i < set_count; i++) {
		uniform_descriptors_total += layouts[i].number_of_uniforms;
		texture_descriptors_total += layouts[i].number_of_textures;
		storage_image_descriptors_total += layouts[i].number_of_storage_images;
		ssbo_descriptors_total += layouts[i].number_of_ssbos;
	}

	VkDescriptorPoolSize pool_sizes[4] = {{0}};
	unsigned int number_of_pool_sizes = 0;

	if(uniform_descriptors_total) {
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = uniform_descriptors_total;
		number_of_pool_sizes++;
	}

	if(texture_descriptors_total) {
		pool_sizes[number_of_pool_sizes].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[number_of_pool_sizes++].descriptorCount = texture_descriptors_total;
	}

	if(storage_image_descriptors_total) {
		pool_sizes[number_of_pool_sizes].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		pool_sizes[number_of_pool_sizes++].descriptorCount = storage_image_descriptors_total;
	}

	if(ssbo_descriptors_total) {
		pool_sizes[number_of_pool_sizes].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pool_sizes[number_of_pool_sizes++].descriptorCount = ssbo_descriptors_total;
	}

	VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	poolInfo.poolSizeCount = number_of_pool_sizes;
	poolInfo.pPoolSizes = pool_sizes;
	poolInfo.maxSets = set_count;

	ASSERT__1(vkCreateDescriptorPool(vkdev, &poolInfo, NULL, &pool->vk_descriptor_pool) == VK_SUCCESS, "vkCreateDescriptorPool error");

	return 0;
}

int pigeon_vulkan_create_descriptor_pool(PigeonVulkanDescriptorPool* pool, 
	unsigned int set_count, PigeonVulkanDescriptorLayout* layouts)
{
	assert(pool && layouts && set_count > 0);

	ASSERT_1(!create_pool(pool, set_count, layouts));

	VkDescriptorSetLayout* layouts_vk = malloc(sizeof *layouts_vk * set_count);
	if(!layouts_vk) {
		vkDestroyDescriptorPool(vkdev, pool->vk_descriptor_pool, NULL);
		return 1;
	}

	for(unsigned int i = 0; i < set_count; i++) {
		layouts_vk[i] = layouts[i].handle;
	}

	VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocInfo.descriptorPool = pool->vk_descriptor_pool;
	allocInfo.descriptorSetCount = set_count;
	allocInfo.pSetLayouts = layouts_vk;

	pool->number_of_sets = set_count;
	if(set_count > 1) {
		pool->vk_descriptor_sets = malloc(sizeof *pool->vk_descriptor_sets * set_count);
		if(!pool->vk_descriptor_sets) {
			free(layouts_vk);
			vkDestroyDescriptorPool(vkdev, pool->vk_descriptor_pool, NULL);
			return 1;
		}
	}

	ASSERT__1(vkAllocateDescriptorSets(vkdev, &allocInfo, 
		set_count > 1 ? pool->vk_descriptor_sets : &pool->vk_descriptor_set
	) == VK_SUCCESS, "vkAllocateDescriptorSets error");

	free(layouts_vk);
	return 0;

}

static void do_update(PigeonVulkanDescriptorPool* pool, 
	unsigned int set, unsigned int binding, unsigned int array_index,
	VkDescriptorBufferInfo * buffer_info, VkDescriptorImageInfo * image_info,
	bool buffer_is_ssbo, bool image_is_storage)
{
	assert(!buffer_info || !image_info);

	VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
	descriptor_write.dstSet = pool->number_of_sets > 1 ? pool->vk_descriptor_sets[set] : pool->vk_descriptor_set;
	descriptor_write.dstBinding = binding;
	descriptor_write.dstArrayElement = array_index;
	if(image_info) {
		descriptor_write.descriptorType = image_is_storage ?
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}
	else {
		descriptor_write.descriptorType = buffer_is_ssbo ?
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}
	descriptor_write.descriptorCount = 1;
	descriptor_write.pImageInfo = image_info;
	descriptor_write.pBufferInfo = buffer_info;

	vkUpdateDescriptorSets(vkdev, 1, &descriptor_write, 0, NULL);
}

void pigeon_vulkan_set_descriptor_uniform_buffer(PigeonVulkanDescriptorPool* pool, 
	unsigned int set, unsigned int binding, unsigned int array_index, PigeonVulkanBuffer* buffer)
{
	pigeon_vulkan_set_descriptor_uniform_buffer2(pool, 
	set, binding, array_index, buffer, 0, buffer->size);
}

void pigeon_vulkan_set_descriptor_uniform_buffer2(PigeonVulkanDescriptorPool* pool, 
	unsigned int set, unsigned int binding, unsigned int array_index, PigeonVulkanBuffer* buffer,
	uint64_t offset, uint64_t range)
{
	assert(pool && set < pool->number_of_sets);
	assert(buffer && buffer->vk_buffer);

	VkDescriptorBufferInfo buffer_info = {0};
	buffer_info.buffer = buffer->vk_buffer;
	buffer_info.offset = offset;
	buffer_info.range = range;

	do_update(pool, set, binding, array_index, &buffer_info, NULL, false, false);
}

void pigeon_vulkan_set_descriptor_ssbo(PigeonVulkanDescriptorPool* pool, 
	unsigned int set, unsigned int binding, unsigned int array_index, PigeonVulkanBuffer* buffer)
{
	pigeon_vulkan_set_descriptor_ssbo2(pool, 
	set, binding, array_index, buffer, 0, buffer->size);
}

void pigeon_vulkan_set_descriptor_ssbo2(PigeonVulkanDescriptorPool* pool, 
	unsigned int set, unsigned int binding, unsigned int array_index, PigeonVulkanBuffer* buffer,
	uint64_t offset, uint64_t range)
{
	assert(pool && set < pool->number_of_sets);
	assert(buffer && buffer->vk_buffer);

	VkDescriptorBufferInfo buffer_info = {0};
	buffer_info.buffer = buffer->vk_buffer;
	buffer_info.offset = offset;
	buffer_info.range = range;

	do_update(pool, set, binding, array_index, &buffer_info, NULL, true, false);
}

void pigeon_vulkan_set_descriptor_texture(PigeonVulkanDescriptorPool* pool, 
	unsigned int set, unsigned int binding, unsigned int array_index,
	PigeonVulkanImageView* image_view, PigeonVulkanSampler* sampler)
{
	assert(pool && set < pool->number_of_sets);
	assert(image_view && image_view->vk_image_view && sampler && sampler->vk_sampler);

	VkDescriptorImageInfo image_info = {0};
	image_info.imageLayout = 
		pigeon_wgi_image_format_is_depth(image_view->format) ? 
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView = image_view->vk_image_view;
	image_info.sampler = sampler->vk_sampler;

	do_update(pool, set, binding, array_index, NULL, &image_info, false, false);

}

void pigeon_vulkan_set_descriptor_storage_image(PigeonVulkanDescriptorPool* pool, 
	unsigned int set, unsigned int binding, unsigned int array_index,
	PigeonVulkanImageView* image_view)
{
	assert(pool && set < pool->number_of_sets);
	assert(image_view && image_view->vk_image_view);

	VkDescriptorImageInfo image_info = {0};
	image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	image_info.imageView = image_view->vk_image_view;

	do_update(pool, set, binding, array_index, NULL, &image_info, false, true);
}

void pigeon_vulkan_destroy_descriptor_pool(PigeonVulkanDescriptorPool* pool)
{
	if (pool && pool->vk_descriptor_pool) {
		vkDestroyDescriptorPool(vkdev, pool->vk_descriptor_pool, NULL);
		pool->vk_descriptor_pool = NULL;

		if(pool->number_of_sets > 1) free(pool->vk_descriptor_sets);
	}
	else {
		assert(false);
	}
}
