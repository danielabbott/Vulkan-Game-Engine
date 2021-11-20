#pragma once

#include "buffer.h"
#include "descriptor.h"
#include "fence.h"
#include "framebuffer.h"
#include "image.h"
#include "pipeline.h"
#include "query.h"
#include "renderpass.h"
#include "semaphore.h"
#include <stdbool.h>
#include <stdint.h>

struct VkCommandPool_T;
struct VkCommandBuffer_T;

typedef struct PigeonVulkanCommandPool {
	struct VkCommandPool_T* vk_command_pool;

	unsigned int buffer_count;

	union {
		struct VkCommandBuffer_T* vk_command_buffer; // if buffer_count = 1
		struct VkCommandBuffer_T** vk_command_buffers; // if buffer_count > 1
	};

	bool primary;
	bool use_transfer_queue;
	bool one_shot; // command buffers used once then pool is reset.

	// These are only valid when buffer_count == 1
	bool recording;
	bool recorded;
} PigeonVulkanCommandPool;

PIGEON_ERR_RET pigeon_vulkan_create_command_pool(
	PigeonVulkanCommandPool*, unsigned int buffer_count, bool primary, bool use_transfer_queue, bool one_shot);

PIGEON_ERR_RET pigeon_vulkan_start_submission(PigeonVulkanCommandPool*, unsigned int buffer_index);
int pigeon_vulkan_start_submission2(PigeonVulkanCommandPool* command_pool, unsigned int buffer_index,
	PigeonVulkanRenderPass* render_pass, PigeonVulkanFramebuffer* framebuffer);

void pigeon_vulkan_wait_for_vertex_data_transfer(PigeonVulkanCommandPool*, unsigned int buffer_index);
void pigeon_vulkan_transition_image_preinit_to_shader_read(
	PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanImage*);
void pigeon_vulkan_transition_image_to_transfer_dst(
	PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanImage*);
void pigeon_vulkan_transition_transfer_src_to_shader_read(
	PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanImage*);
void pigeon_vulkan_transition_transfer_dst_to_shader_read(
	PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanImage*);

void pigeon_vulkan_clear_image(
	PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanImage*, float r, float g, float b, float a);
void pigeon_vulkan_clear_depth_image(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanImage*, float d);

void pigeon_vulkan_transfer_buffer_to_image(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanBuffer*,
	uint64_t buffer_offset, PigeonVulkanImage*, unsigned int array_layer, unsigned int image_x, unsigned int image_y,
	unsigned int image_width, unsigned int image_height, unsigned int mip_levels);

void pigeon_vulkan_set_viewport_size(
	PigeonVulkanCommandPool*, unsigned int buffer_index, unsigned int width, unsigned int height);

void pigeon_vulkan_start_render_pass(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanRenderPass*,
	PigeonVulkanFramebuffer*, unsigned int viewport_width, unsigned int viewport_height,
	bool using_secondary_buffers // Drawing commands are in a secondary command buffer (not this primary one)
);

void pigeon_vulkan_end_render_pass(PigeonVulkanCommandPool*, unsigned int buffer_index);

void pigeon_vulkan_execute_secondary(PigeonVulkanCommandPool*, unsigned int buffer_index,
	PigeonVulkanCommandPool* secondary_command_pool, unsigned int secondary_buffer_index);

void pigeon_vulkan_wait_for_depth_write(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanImage*);
void pigeon_vulkan_wait_for_colour_write(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanImage*);

void pigeon_vulkan_wait_and_blit_image(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanImage* src,
	PigeonVulkanImage* dst, bool bilinear_filter);

void pigeon_vulkan_bind_pipeline(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanPipeline*);
void pigeon_vulkan_bind_descriptor_set(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanPipeline*,
	PigeonVulkanDescriptorPool*, unsigned int set);
void pigeon_vulkan_bind_vertex_buffers(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanBuffer*,
	unsigned int attribute_count, uint64_t* offsets);
void pigeon_vulkan_bind_index_buffer(
	PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanBuffer*, uint64_t offset, bool big_indices);
void pigeon_vulkan_draw(PigeonVulkanCommandPool*, unsigned int buffer_index, unsigned int first, unsigned int vertices,
	unsigned int instances, PigeonVulkanPipeline*, unsigned int push_constants_size, void* push_constants_data);
void pigeon_vulkan_draw_indexed(PigeonVulkanCommandPool*, unsigned int buffer_index, uint32_t start_vertex,
	uint32_t first, uint32_t indices, uint32_t instances, PigeonVulkanPipeline*, uint32_t push_constants_size,
	void* push_constants_data);

void pigeon_vulkan_multidraw_indexed(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanPipeline*,
	unsigned int push_constants_size, void* push_constants_data, PigeonVulkanBuffer*, uint64_t buffer_offset,
	uint32_t first_multidraw_index, uint32_t multidraw_cmd_count);

void pigeon_vulkan_buffer_transfer(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanBuffer* dst,
	uint64_t dst_offset, PigeonVulkanBuffer* src, uint64_t src_offset, uint64_t size);

void pigeon_vulkan_reset_query_pool(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanTimerQueryPool*);
void pigeon_vulkan_set_timer(
	PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanTimerQueryPool*, unsigned int n);

PIGEON_ERR_RET pigeon_vulkan_end_submission(PigeonVulkanCommandPool*, unsigned int buffer_index);

// Fence is optional
PIGEON_ERR_RET pigeon_vulkan_submit(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanFence*);
PIGEON_ERR_RET pigeon_vulkan_submit2(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanFence*,
	PigeonVulkanSemaphore* wait_semaphore, PigeonVulkanSemaphore* signal_semaphore);
PIGEON_ERR_RET pigeon_vulkan_submit3(PigeonVulkanCommandPool*, unsigned int buffer_index, PigeonVulkanFence*,
	PigeonVulkanSemaphore* wait_semaphore, PigeonVulkanSemaphore* wait_semaphore2,
	PigeonVulkanSemaphore* signal_semaphore, PigeonVulkanSemaphore* signal_semaphore2);

typedef enum {
	PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_ALL,
	PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_FRAGMENT,
	PIGEON_VULKAN_SEMAPHORE_WAIT_STAGE_COLOUR_WRITE // for writing to swapchain
} PigeonVulkanWaitSemaphoreStage;

typedef struct PigeonVulkanSubmitInfo {
	PigeonVulkanCommandPool* pool;
	unsigned int buffer_index;

	// Copy the semaphore structs (copies vulkan object pointer)

	PigeonVulkanSemaphore wait_semaphores[8];
	PigeonVulkanWaitSemaphoreStage wait_semaphore_types[8];

	PigeonVulkanSemaphore signal_semaphores[8];

	unsigned int wait_semaphore_count, signal_semaphore_count;
} PigeonVulkanSubmitInfo;

PIGEON_ERR_RET pigeon_vulkan_submit_multi(PigeonVulkanSubmitInfo*, unsigned int count, PigeonVulkanFence*);

// * Wait for execution to complete before resetting or destroying a command pool *

PIGEON_ERR_RET pigeon_vulkan_reset_command_pool(PigeonVulkanCommandPool*);
void pigeon_vulkan_destroy_command_pool(PigeonVulkanCommandPool*);
