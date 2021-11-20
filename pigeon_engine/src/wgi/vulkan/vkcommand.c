#include "singleton.h"
#include <assert.h>
#include <pigeon/assert.h>
#include <pigeon/wgi/vulkan/command.h>
#include <stdlib.h>

PIGEON_ERR_RET pigeon_vulkan_create_command_pool(PigeonVulkanCommandPool* command_pool, unsigned int buffer_count,
	bool primary, bool use_transfer_queue, bool one_shot)
{
	assert(command_pool);
	assert(buffer_count);

	command_pool->one_shot = one_shot;

	if (buffer_count > 1) {
		command_pool->vk_command_buffers = malloc(sizeof *command_pool->vk_command_buffers * buffer_count);
		ASSERT_R1(command_pool->vk_command_buffers);
	}

	VkCommandPoolCreateInfo pool_create = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	pool_create.queueFamilyIndex = singleton_data.general_queue_family;

	if (use_transfer_queue && singleton_data.transfer_queue_family != UINT32_MAX) {
		pool_create.queueFamilyIndex = singleton_data.transfer_queue_family;
	}

	if (one_shot)
		pool_create.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	if (vkCreateCommandPool(vkdev, &pool_create, NULL, &command_pool->vk_command_pool) != VK_SUCCESS) {
		ERRLOG("vkCreateCommandPool error");
		if (buffer_count > 1)
			free(command_pool->vk_command_buffers);
		return 1;
	}

	VkCommandBufferAllocateInfo alloc = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	alloc.commandPool = command_pool->vk_command_pool;
	alloc.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	alloc.commandBufferCount = buffer_count;

	if (vkAllocateCommandBuffers(
			vkdev, &alloc, buffer_count > 1 ? command_pool->vk_command_buffers : &command_pool->vk_command_buffer)
		!= VK_SUCCESS) {
		ERRLOG("vkAllocateCommandBuffers error");
		vkDestroyCommandPool(vkdev, command_pool->vk_command_pool, NULL);
		if (buffer_count > 1)
			free(command_pool->vk_command_buffers);
		return 1;
	}

	command_pool->primary = primary;
	command_pool->use_transfer_queue = use_transfer_queue;
	command_pool->buffer_count = buffer_count;

	return 0;
}

static VkCommandBuffer get_cmd_buf(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index)
{
	return command_pool->buffer_count == 1 ? command_pool->vk_command_buffer
										   : command_pool->vk_command_buffers[buffer_index];
}

PIGEON_ERR_RET pigeon_vulkan_start_submission(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);

	VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

	if (command_pool->one_shot)
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkCommandBufferInheritanceInfo inheritance = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
	if (!command_pool->primary) {
		begin.pInheritanceInfo = &inheritance;
	}

	ASSERT_LOG_R1(vkBeginCommandBuffer(get_cmd_buf(command_pool, buffer_index), &begin) == VK_SUCCESS,
		"vkBeginCommandBuffer error");

	command_pool->recorded = false;
	command_pool->recording = true;
	return 0;
}

int pigeon_vulkan_start_submission2(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanRenderPass* render_pass, PigeonVulkanFramebuffer* framebuffer)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	if (render_pass)
		assert(framebuffer);

	VkCommandBufferBeginInfo begin = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkCommandBufferInheritanceInfo inheritance = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };

	if (!command_pool->primary) {
		if (render_pass) {
			inheritance.renderPass = render_pass->vk_renderpass;
			begin.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

			if (framebuffer) {
				inheritance.framebuffer = framebuffer->vk_framebuffer;
			}
		}
		begin.pInheritanceInfo = &inheritance;
	}

	ASSERT_LOG_R1(vkBeginCommandBuffer(get_cmd_buf(command_pool, buffer_index), &begin) == VK_SUCCESS,
		"vkBeginCommandBuffer error");

	command_pool->recorded = false;
	command_pool->recording = true;
	return 0;
}

void pigeon_vulkan_transition_image_preinit_to_shader_read(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanImage* image)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer && image);
	assert(buffer_index < command_pool->buffer_count);

	VkImageMemoryBarrier image_memory_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	image_memory_barrier.srcAccessMask = 0;
	image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = image_memory_barrier.dstQueueFamilyIndex
		= singleton_data.general_queue_family;
	image_memory_barrier.image = image->vk_image;

	image_memory_barrier.subresourceRange.aspectMask
		= pigeon_wgi_image_format_is_depth(image->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.levelCount = image->mip_levels;
	image_memory_barrier.subresourceRange.layerCount = image->layers;

	vkCmdPipelineBarrier(get_cmd_buf(command_pool, buffer_index),
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // source stage
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // destination stage
		0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
}

void pigeon_vulkan_transition_image_to_transfer_dst(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanImage* image)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer && image);
	assert(buffer_index < command_pool->buffer_count);

	VkImageMemoryBarrier image_memory_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	image_memory_barrier.srcAccessMask = 0;
	image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = image_memory_barrier.dstQueueFamilyIndex
		= singleton_data.general_queue_family;
	image_memory_barrier.image = image->vk_image;

	image_memory_barrier.subresourceRange.aspectMask
		= pigeon_wgi_image_format_is_depth(image->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.levelCount = image->mip_levels;
	image_memory_barrier.subresourceRange.layerCount = image->layers;

	vkCmdPipelineBarrier(get_cmd_buf(command_pool, buffer_index),
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // source stage
		VK_PIPELINE_STAGE_TRANSFER_BIT, // destination stage
		0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
}

void pigeon_vulkan_set_viewport_size(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, unsigned int width, unsigned int height)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(width && height);

	VkViewport viewport = { 0 };
	viewport.width = (float)width;
	viewport.height = (float)height;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(get_cmd_buf(command_pool, buffer_index), 0, 1, &viewport);

	VkRect2D scissor = { 0 };
	scissor.extent.width = width;
	scissor.extent.height = height;
	vkCmdSetScissor(get_cmd_buf(command_pool, buffer_index), 0, 1, &scissor);
}

void pigeon_vulkan_start_render_pass(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanRenderPass* render_pass, PigeonVulkanFramebuffer* framebuffer, unsigned int viewport_width,
	unsigned int viewport_height, bool using_secondary_buffers)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count && render_pass && framebuffer);

	VkRenderPassBeginInfo begin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	begin.renderPass = render_pass->vk_renderpass;
	begin.framebuffer = framebuffer->vk_framebuffer;
	begin.renderArea.extent.width = viewport_width;
	begin.renderArea.extent.height = viewport_height;

	VkClearValue clear_values[3] = { { { { 0 } } } };

	unsigned int i = 0;
	if (render_pass->has_colour_image) {
		i++;
	}
	if (render_pass->has_2_colour_images) {
		i++;
	}
	if (render_pass->has_writeable_depth_image) {
		i++;
	}

	begin.clearValueCount = i;
	begin.pClearValues = clear_values;

	vkCmdBeginRenderPass(get_cmd_buf(command_pool, buffer_index), &begin,
		using_secondary_buffers ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);
}

void pigeon_vulkan_end_render_pass(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);

	vkCmdEndRenderPass(get_cmd_buf(command_pool, buffer_index));
}

void pigeon_vulkan_execute_secondary(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanCommandPool* secondary_command_pool, unsigned int secondary_buffer_index)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(
		secondary_command_pool && secondary_command_pool->vk_command_pool && secondary_command_pool->vk_command_buffer);
	assert(secondary_buffer_index < secondary_command_pool->buffer_count);

	VkCommandBuffer secondary_cmd_buf = get_cmd_buf(secondary_command_pool, secondary_buffer_index);

	vkCmdExecuteCommands(get_cmd_buf(command_pool, buffer_index), 1, &secondary_cmd_buf);
}

void pigeon_vulkan_wait_for_depth_write(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanImage* image)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(image && image->vk_image && pigeon_wgi_image_format_is_depth(image->format));

	VkImageMemoryBarrier image_memory_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	image_memory_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.oldLayout = image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = image_memory_barrier.dstQueueFamilyIndex
		= singleton_data.general_queue_family;
	image_memory_barrier.image = image->vk_image;

	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	image_memory_barrier.subresourceRange.levelCount = image->mip_levels;
	image_memory_barrier.subresourceRange.layerCount = image->layers;

	vkCmdPipelineBarrier(get_cmd_buf(command_pool, buffer_index),
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // source stage
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // destination stage
		0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
}

void pigeon_vulkan_wait_for_colour_write(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanImage* image)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(image && image->vk_image && !pigeon_wgi_image_format_is_depth(image->format));

	VkImageMemoryBarrier image_memory_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.oldLayout = image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = image_memory_barrier.dstQueueFamilyIndex
		= singleton_data.general_queue_family;
	image_memory_barrier.image = image->vk_image;

	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.levelCount = image->mip_levels;
	image_memory_barrier.subresourceRange.layerCount = image->layers;

	vkCmdPipelineBarrier(get_cmd_buf(command_pool, buffer_index),
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // source stage
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // destination stage
		0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
}

void pigeon_vulkan_wait_for_vertex_data_transfer(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);

	VkMemoryBarrier memory_barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	memory_barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;

	vkCmdPipelineBarrier(get_cmd_buf(command_pool, buffer_index),
		VK_PIPELINE_STAGE_TRANSFER_BIT, // source stage
		VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, // destination stage
		0, 1, &memory_barrier, 0, NULL, 0, NULL);
}

// Bilinear filtering will look bad if downsampling an image more than 2x
void pigeon_vulkan_wait_and_blit_image(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanImage* src, PigeonVulkanImage* dst, bool bilinear_filter)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(src && src->vk_image && dst && dst->vk_image);

	VkImageMemoryBarrier image_memory_barriers[2]
		= { { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER }, { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER } };

	// Wait for write to src, ready for transfer

	image_memory_barriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	image_memory_barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	image_memory_barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	image_memory_barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	image_memory_barriers[0].srcQueueFamilyIndex = image_memory_barriers[0].dstQueueFamilyIndex
		= singleton_data.general_queue_family;
	image_memory_barriers[0].image = src->vk_image;

	image_memory_barriers[0].subresourceRange.aspectMask
		= pigeon_wgi_image_format_is_depth(src->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barriers[0].subresourceRange.levelCount = src->mip_levels;
	image_memory_barriers[0].subresourceRange.layerCount = src->layers;

	// Prepare dst for writing

	image_memory_barriers[1].srcAccessMask = 0;
	image_memory_barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	image_memory_barriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_memory_barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_memory_barriers[1].srcQueueFamilyIndex = image_memory_barriers[1].dstQueueFamilyIndex
		= singleton_data.general_queue_family;
	image_memory_barriers[1].image = dst->vk_image;

	image_memory_barriers[1].subresourceRange.aspectMask
		= pigeon_wgi_image_format_is_depth(dst->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barriers[1].subresourceRange.levelCount = dst->mip_levels;
	image_memory_barriers[1].subresourceRange.layerCount = dst->layers;

	vkCmdPipelineBarrier(get_cmd_buf(command_pool, buffer_index),
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // source stage
		VK_PIPELINE_STAGE_TRANSFER_BIT, // destination stage
		0, 0, NULL, 0, NULL, 2, image_memory_barriers);

	VkImageBlit region = { 0 };
	region.srcSubresource.aspectMask
		= pigeon_wgi_image_format_is_depth(src->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource.aspectMask
		= pigeon_wgi_image_format_is_depth(dst->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;

	region.srcOffsets[1].x = (int32_t)src->width;
	region.srcOffsets[1].y = (int32_t)src->height;
	region.srcOffsets[1].z = 1;
	region.dstOffsets[1].x = (int32_t)dst->width;
	region.dstOffsets[1].y = (int32_t)dst->height;
	region.dstOffsets[1].z = 1;

	vkCmdBlitImage(get_cmd_buf(command_pool, buffer_index), src->vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region,
		bilinear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST);
}

void pigeon_vulkan_transition_transfer_src_to_shader_read(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanImage* image)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(image && image->vk_image);

	VkImageMemoryBarrier image_memory_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	image_memory_barrier.srcAccessMask = 0;
	image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = image_memory_barrier.dstQueueFamilyIndex
		= singleton_data.general_queue_family;
	image_memory_barrier.image = image->vk_image;

	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.levelCount = image->mip_levels;
	image_memory_barrier.subresourceRange.layerCount = image->layers;

	vkCmdPipelineBarrier(get_cmd_buf(command_pool, buffer_index),
		VK_PIPELINE_STAGE_TRANSFER_BIT, // source stage
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // destination stage
		0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
}

void pigeon_vulkan_transition_transfer_dst_to_shader_read(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanImage* image)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(image && image->vk_image);

	VkImageMemoryBarrier image_memory_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_memory_barrier.newLayout = pigeon_wgi_image_format_is_depth(image->format)
		? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_memory_barrier.srcQueueFamilyIndex = image_memory_barrier.dstQueueFamilyIndex
		= singleton_data.general_queue_family;
	image_memory_barrier.image = image->vk_image;

	image_memory_barrier.subresourceRange.aspectMask
		= pigeon_wgi_image_format_is_depth(image->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.levelCount = image->mip_levels;
	image_memory_barrier.subresourceRange.layerCount = image->layers;

	vkCmdPipelineBarrier(get_cmd_buf(command_pool, buffer_index),
		VK_PIPELINE_STAGE_TRANSFER_BIT, // source stage
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // destination stage
		0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
}

void pigeon_vulkan_transfer_buffer_to_image(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanBuffer* buffer, uint64_t buffer_offset, PigeonVulkanImage* image, unsigned int array_layer,
	unsigned int image_x, unsigned int image_y, unsigned int image_width, unsigned int image_height,
	unsigned int mip_levels)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(buffer && buffer->vk_buffer && buffer_offset < buffer->size);
	assert(image && image->vk_image);
	assert(image_x + image_width <= image->width);
	assert(image_y + image_height <= image->height);
	if (!mip_levels)
		mip_levels = 1;
	assert(mip_levels <= image->mip_levels && mip_levels <= 13);

	uint64_t o = buffer_offset;
	unsigned int size = (image_width * image_height * pigeon_image_format_bytes_per_4x4_block(image->format)) / 16;

	VkBufferImageCopy regions[13] = { { 0 } };
	for (unsigned int mip = 0; mip < mip_levels; mip++) {
		assert(o + size <= buffer->size);

		regions[mip].bufferOffset = o;
		regions[mip].imageSubresource.aspectMask
			= pigeon_wgi_image_format_is_depth(image->format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		regions[mip].imageSubresource.mipLevel = mip;
		regions[mip].imageSubresource.layerCount = 1;
		regions[mip].imageSubresource.baseArrayLayer = array_layer;
		regions[mip].imageOffset.x = (int32_t)image_x;
		regions[mip].imageOffset.y = (int32_t)image_y;
		regions[mip].imageExtent.width = image_width;
		regions[mip].imageExtent.height = image_height;
		regions[mip].imageExtent.depth = 1;

		o += size;
		size /= 4;
		image_width /= 2;
		image_height /= 2;
		image_x /= 2;
		image_y /= 2;
	}

	vkCmdCopyBufferToImage(get_cmd_buf(command_pool, buffer_index), buffer->vk_buffer, image->vk_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels, regions);
}

void pigeon_vulkan_clear_image(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanImage* image, float r, float g, float b, float a)
{
	VkClearColorValue clearc = { { 0 } };
	clearc.float32[0] = r;
	clearc.float32[1] = g;
	clearc.float32[2] = b;
	clearc.float32[3] = a;

	VkImageSubresourceRange range = { 0 };
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.levelCount = image->mip_levels;
	range.layerCount = image->layers;

	vkCmdClearColorImage(get_cmd_buf(command_pool, buffer_index), image->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		&clearc, 1, &range);
}

void pigeon_vulkan_clear_depth_image(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanImage* image, float d)
{
	VkClearDepthStencilValue clear = { 0 };
	clear.depth = d;

	VkImageSubresourceRange range = { 0 };
	range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	range.levelCount = image->mip_levels;
	range.layerCount = 1;

	vkCmdClearDepthStencilImage(get_cmd_buf(command_pool, buffer_index), image->vk_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear, 1, &range);
}

void pigeon_vulkan_bind_pipeline(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanPipeline* pipeline)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(pipeline && pipeline->vk_pipeline);

	vkCmdBindPipeline(get_cmd_buf(command_pool, buffer_index), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vk_pipeline);
}

void pigeon_vulkan_bind_descriptor_set(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanPipeline* pipeline, PigeonVulkanDescriptorPool* pool, unsigned int set)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(pool && pool->vk_descriptor_pool);
	assert(pipeline && pipeline->vk_pipeline_layout);

	vkCmdBindDescriptorSets(get_cmd_buf(command_pool, buffer_index), VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline->vk_pipeline_layout, 0, 1,
		pool->number_of_sets > 1 ? &pool->vk_descriptor_sets[set] : &pool->vk_descriptor_set, 0, NULL);
}

void pigeon_vulkan_bind_vertex_buffers(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanBuffer* buffer, unsigned int attribute_count, uint64_t* offsets)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(buffer && buffer->vk_buffer);
	assert(offsets && attribute_count && attribute_count <= PIGEON_WGI_MAX_VERTEX_ATTRIBUTES);

	VkBuffer buffers[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES];
	for (unsigned int i = 0; i < attribute_count; i++)
		buffers[i] = buffer->vk_buffer;

	vkCmdBindVertexBuffers(get_cmd_buf(command_pool, buffer_index), 0, attribute_count, buffers, offsets);
}

void pigeon_vulkan_bind_index_buffer(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanBuffer* buffer, uint64_t offset, bool big_indices)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(buffer && buffer->vk_buffer);
	assert(offset <= buffer->size);

	vkCmdBindIndexBuffer(get_cmd_buf(command_pool, buffer_index), buffer->vk_buffer, offset,
		big_indices ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
}

static void set_push_constants(VkCommandBuffer cmd_buf, PigeonVulkanPipeline* pipeline,
	unsigned int push_constants_size, void* push_constants_data)
{
	if (push_constants_data && push_constants_size) {
		vkCmdPushConstants(cmd_buf, pipeline->vk_pipeline_layout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, push_constants_size, push_constants_data);
	}
}

void pigeon_vulkan_draw(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, unsigned int first,
	unsigned int vertices, unsigned int instances, PigeonVulkanPipeline* pipeline, unsigned int push_constants_size,
	void* push_constants_data)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(!push_constants_size || push_constants_data);
	assert(pipeline && pipeline->vk_pipeline_layout);
	if (!instances)
		instances = 1;

	VkCommandBuffer cmd_buf = get_cmd_buf(command_pool, buffer_index);
	set_push_constants(cmd_buf, pipeline, push_constants_size, push_constants_data);
	vkCmdDraw(cmd_buf, vertices, instances, first, 0);
}

void pigeon_vulkan_draw_indexed(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, uint32_t start_vertex,
	unsigned int first, unsigned int indices, unsigned int instances, PigeonVulkanPipeline* pipeline,
	unsigned int push_constants_size, void* push_constants_data)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(!push_constants_size || push_constants_data);
	assert(pipeline && pipeline->vk_pipeline_layout);
	if (!instances)
		instances = 1;

	VkCommandBuffer cmd_buf = get_cmd_buf(command_pool, buffer_index);
	set_push_constants(cmd_buf, pipeline, push_constants_size, push_constants_data);
	vkCmdDrawIndexed(cmd_buf, indices, instances, first, (int32_t)start_vertex, 0);
}

void pigeon_vulkan_multidraw_indexed(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanPipeline* pipeline, unsigned int push_constants_size, void* push_constants_data,
	PigeonVulkanBuffer* buffer, uint64_t buffer_offset, uint32_t first_multidraw_index, uint32_t multidraw_cmd_count)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);
	assert(!push_constants_size || push_constants_data);
	assert(pipeline && pipeline->vk_pipeline_layout);
	assert((uint64_t)buffer_offset
			+ ((uint64_t)first_multidraw_index + (uint64_t)multidraw_cmd_count) * sizeof(VkDrawIndexedIndirectCommand)
		<= buffer->size);

	VkCommandBuffer cmd_buf = get_cmd_buf(command_pool, buffer_index);
	set_push_constants(cmd_buf, pipeline, push_constants_size, push_constants_data);

	vkCmdDrawIndexedIndirect(cmd_buf, buffer->vk_buffer,
		buffer_offset + first_multidraw_index * sizeof(VkDrawIndexedIndirectCommand), multidraw_cmd_count,
		sizeof(VkDrawIndexedIndirectCommand));
}

void pigeon_vulkan_buffer_transfer(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanBuffer* dst, uint64_t dst_offset, PigeonVulkanBuffer* src, uint64_t src_offset, uint64_t size)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count && size);
	assert(dst && dst->vk_buffer && dst_offset + size <= dst->size);
	assert(src && src->vk_buffer && src_offset + size <= src->size);

	VkBufferCopy region = { 0 };
	region.srcOffset = src_offset;
	region.dstOffset = dst_offset;
	region.size = size;
	vkCmdCopyBuffer(get_cmd_buf(command_pool, buffer_index), src->vk_buffer, dst->vk_buffer, 1, &region);
}

void pigeon_vulkan_reset_query_pool(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanTimerQueryPool* pool)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(pool && pool->vk_query_pool);

	vkCmdResetQueryPool(get_cmd_buf(command_pool, buffer_index), pool->vk_query_pool, 0, pool->num_query_objects);
}

void pigeon_vulkan_set_timer(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanTimerQueryPool* pool, unsigned int n)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(pool && pool->vk_query_pool && n <= pool->num_query_objects);

	if (!command_pool->use_transfer_queue && singleton_data.general_queue_timestamp_bits_valid) {
		vkCmdWriteTimestamp(
			get_cmd_buf(command_pool, buffer_index), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool->vk_query_pool, n);
	}
	if (command_pool->use_transfer_queue && singleton_data.transfer_queue_timestamp_bits_valid) {
		vkCmdWriteTimestamp(
			get_cmd_buf(command_pool, buffer_index), VK_PIPELINE_STAGE_TRANSFER_BIT, pool->vk_query_pool, n);
	}
}

PIGEON_ERR_RET pigeon_vulkan_end_submission(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index)
{
	assert(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	assert(buffer_index < command_pool->buffer_count);

	ASSERT_LOG_R1(
		vkEndCommandBuffer(get_cmd_buf(command_pool, buffer_index)) == VK_SUCCESS, "vkEndCommandBuffer error");
	if (command_pool->buffer_count == 1) {
		command_pool->recorded = true;
		command_pool->recording = false;
	}
	return 0;
}

PIGEON_ERR_RET pigeon_vulkan_submit_multi(PigeonVulkanSubmitInfo* info, unsigned int count, PigeonVulkanFence* fence)
{
	ASSERT_R1(info && count <= 12);
	if (fence)
		ASSERT_R1(fence->vk_fence);
	if (!count)
		return 0;

	bool use_transfer_queue = info[0].pool->use_transfer_queue;

	VkSubmitInfo submits[12] = { { 0 } };

	for (unsigned int i = 0; i < count; i++) {
		ASSERT_R1(info[i].pool->use_transfer_queue == use_transfer_queue);

		submits[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		unsigned int buffer_count = info[i].pool->buffer_count;
		submits[i].commandBufferCount = buffer_count;
		submits[i].pCommandBuffers
			= buffer_count > 1 ? info[i].pool->vk_command_buffers : &info[i].pool->vk_command_buffer;

		submits[i].pWaitSemaphores = (const VkSemaphore*)info[i].wait_semaphores;
		submits[i].pWaitDstStageMask = info[i].wait_semaphore_types; // TODO change to vulkan flags
		submits[i].waitSemaphoreCount = info[i].wait_semaphore_count;
		submits[i].pSignalSemaphores = (const VkSemaphore*)info[i].signal_semaphores;
		submits[i].signalSemaphoreCount = info[i].signal_semaphore_count;

		ASSERT_R1(info[i].wait_semaphore_count <= 8);
		ASSERT_R1(info[i].signal_semaphore_count && info[i].wait_semaphore_count <= 8);

		for (unsigned int j = 0; j < info[i].wait_semaphore_count; j++) {
			if (info[i].wait_semaphore_types[j] == PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_ALL) {
				*(VkPipelineStageFlagBits*)(void*)&info[i].wait_semaphore_types[j] = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			} else if (info[i].wait_semaphore_types[j] == PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_FRAGMENT) {
				*(VkPipelineStageFlagBits*)(void*)&info[i].wait_semaphore_types[j]
					= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			} else if (info[i].wait_semaphore_types[j] == PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_COLOUR_WRITE) {
				*(VkPipelineStageFlagBits*)(void*)&info[i].wait_semaphore_types[j]
					= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
		}
	}

	ASSERT_LOG_R1(vkQueueSubmit(use_transfer_queue ? singleton_data.transfer_queue : singleton_data.general_queue,
					  count, submits, fence ? fence->vk_fence : VK_NULL_HANDLE)
			== VK_SUCCESS,
		"vkQueueSubmit error");
	return 0;
}

PIGEON_ERR_RET pigeon_vulkan_submit3(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanFence* fence, PigeonVulkanSemaphore* wait_semaphore, PigeonVulkanSemaphore* wait_semaphore2,
	PigeonVulkanSemaphore* signal_semaphore, PigeonVulkanSemaphore* signal_semaphore2)
{
	ASSERT_R1(command_pool && command_pool->vk_command_pool && command_pool->vk_command_buffer);
	ASSERT_R1(buffer_index < command_pool->buffer_count);

	ASSERT_LOG_R1(
		vkEndCommandBuffer(get_cmd_buf(command_pool, buffer_index)) == VK_SUCCESS, "vkEndCommandBuffer error");

	VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit.commandBufferCount = 1;

	VkCommandBuffer cmd_buf = get_cmd_buf(command_pool, buffer_index);
	submit.pCommandBuffers = &cmd_buf;

	VkSemaphore signal_semaphores[2];

	if (signal_semaphore) {
		submit.signalSemaphoreCount++;
		signal_semaphores[0] = signal_semaphore->vk_semaphore;
	}
	if (signal_semaphore2) {
		signal_semaphores[submit.signalSemaphoreCount++] = signal_semaphore2->vk_semaphore;
	}

	if (signal_semaphore || signal_semaphore2)
		submit.pSignalSemaphores = signal_semaphores;

	VkSemaphore wait_semaphores[2];
	VkPipelineStageFlags stage[2] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

	if (wait_semaphore) {
		submit.waitSemaphoreCount++;
		wait_semaphores[0] = wait_semaphore->vk_semaphore;
	}
	if (wait_semaphore2) {
		wait_semaphores[submit.waitSemaphoreCount++] = wait_semaphore2->vk_semaphore;
	}

	if (wait_semaphore || wait_semaphore2) {
		submit.pWaitSemaphores = wait_semaphores;
		submit.pWaitDstStageMask = stage;
	}

	ASSERT_LOG_R1(
		vkQueueSubmit(command_pool->use_transfer_queue ? singleton_data.transfer_queue : singleton_data.general_queue,
			1, &submit, fence ? fence->vk_fence : VK_NULL_HANDLE)
			== VK_SUCCESS,
		"vkQueueSubmit error");
	return 0;
}

PIGEON_ERR_RET pigeon_vulkan_submit2(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanFence* fence, PigeonVulkanSemaphore* wait_semaphore, PigeonVulkanSemaphore* signal_semaphore)
{
	return pigeon_vulkan_submit3(command_pool, buffer_index, fence, wait_semaphore, NULL, signal_semaphore, NULL);
}

PIGEON_ERR_RET pigeon_vulkan_submit(
	PigeonVulkanCommandPool* command_pool, unsigned int buffer_index, PigeonVulkanFence* fence)
{
	return pigeon_vulkan_submit3(command_pool, buffer_index, fence, NULL, NULL, NULL, NULL);
}

PIGEON_ERR_RET pigeon_vulkan_reset_command_pool(PigeonVulkanCommandPool* command_pool)
{
	assert(command_pool && command_pool->vk_command_pool);

	ASSERT_LOG_R1(
		vkResetCommandPool(vkdev, command_pool->vk_command_pool, 0) == VK_SUCCESS, "vkResetCommandPool error");
	command_pool->recorded = command_pool->recording = false;
	return 0;
}

void pigeon_vulkan_destroy_command_pool(PigeonVulkanCommandPool* command_pool)
{
	assert(command_pool);

	if (command_pool && command_pool->vk_command_pool) {
		vkDestroyCommandPool(vkdev, command_pool->vk_command_pool, NULL);
		command_pool->vk_command_pool = NULL;

		if (command_pool->buffer_count > 1)
			free(command_pool->vk_command_buffers);
		command_pool->vk_command_buffer = NULL;
	}
}
