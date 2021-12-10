#include "Device.h"
#include "CommandPool.h"
#include "../BetterAssert.h"
#include "Include.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "Pipeline.h"
#include "Buffer.h"
#include "Fence.h"
#include "DescriptorSets.h"
#include "QueryPool.h"
#include <array>

using namespace std;

namespace Vulkan {

	CommandPool::CommandPool(Device& d, Queue const& queue, bool create_fence, bool secondary_)
		:device(d), queue_family_index(queue.get_family_index()), queue_handle(queue.get_queue_handle()), secondary(secondary_)
	{


		VkCommandPoolCreateInfo pool_create{};
		pool_create.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_create.queueFamilyIndex = queue_family_index;
		pool_create.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		assert__(vkCreateCommandPool(device.get_device_handle_(), &pool_create, nullptr, &command_pool) == VK_SUCCESS, "vkCreateCommandPool error");
	
		if(create_fence) {
			complete_fence = make_unique<Vulkan::Fence>(device, false);
		}
	}
	bool CommandPool::is_complete()
	{
		assert__(complete_fence, "Command pool was created without a fence. Completion status cannot be known host-side");
		return complete_fence->poll();
	}
	void CommandPool::wait() const
	{
		assert__(complete_fence, "Command pool was created without a fence");
		complete_fence->wait();
	}

	CommandPool::~CommandPool()
	{
		if (cmd_buf) {
			vkFreeCommandBuffers(device.get_device_handle_(), command_pool, 1, &cmd_buf);
			cmd_buf = nullptr;
		}
		if (command_pool) {
			vkDestroyCommandPool(device.get_device_handle_(), command_pool, nullptr);
			command_pool = nullptr;
		}
	}

	void CommandPool::start_submit(optional<reference_wrapper<const RenderPass>> render_pass,
		optional<reference_wrapper<const Framebuffer>> framebuffer)
	{
		if (cmd_buf) {
			vkResetCommandPool(device.get_device_handle_(), command_pool, 0);
			complete_fence->reset();
		}
		else {
			VkCommandBufferAllocateInfo alloc{};
			alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			alloc.commandPool = command_pool;
			alloc.level = secondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			alloc.commandBufferCount = 1;

			assert__(vkAllocateCommandBuffers(device.get_device_handle_(), &alloc, &cmd_buf) == VK_SUCCESS, "vkAllocateCommandBuffers error");
		}

		VkCommandBufferBeginInfo begin {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkCommandBufferInheritanceInfo inheritance {VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO};


		if(secondary) {
			if(render_pass.has_value()) {
				inheritance.renderPass = render_pass.value().get().get_handle();
				begin.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

				if(framebuffer.has_value()) {
					inheritance.framebuffer = framebuffer.value().get().get_handle();
				}
			}
			begin.pInheritanceInfo = &inheritance;
		}


		assert__(vkBeginCommandBuffer(cmd_buf, &begin) == VK_SUCCESS, "vkBeginCommandBuffer error");

	}

	void CommandPool::set_viewport_size(unsigned int w, unsigned int h)
	{
		assert__(cmd_buf, "start_submit has not been called");
		
		VkViewport viewport{};
		viewport.width = static_cast<float>(w);
		viewport.height = static_cast<float>(h);
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd_buf, 0, 1, &viewport);


		VkRect2D scissor{};
		scissor.extent = VkExtent2D{ w, h };
		vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
		
	}

	void CommandPool::reset_query_pool(QueryPool& query_pool)
	{
		assert__(cmd_buf, "start_submit has not been called");

		vkCmdResetQueryPool(cmd_buf, query_pool.get_handle_(), 0, query_pool.get_number_of_queries());
	}
	
	void CommandPool::set_timer(QueryPool& query_pool, unsigned int n)
	{
		assert__(cmd_buf, "start_submit has not been called");

		if(device.get_general_queue().get_timestamp_bits_valid()) {
			vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, query_pool.get_handle_(), n);
		}
	}
	
	void CommandPool::wait_for_depth_write(Image const& image)
	{
		assert__(cmd_buf, "start_submit has not been called");
		assert__(image.get_format() == WGI::ImageFormat::DepthF32, "Not a depth image");

		VkImageMemoryBarrier image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		image_memory_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		image_memory_barrier.oldLayout = image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex = image_memory_barrier.dstQueueFamilyIndex = device.get_general_queue().get_family_index();
		image_memory_barrier.image = image.get_handle();

		image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		image_memory_barrier.subresourceRange.baseMipLevel = 0;
		image_memory_barrier.subresourceRange.levelCount = 1;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			cmd_buf,
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // source stage
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // destination stage
			0,
			0, nullptr,
			0, nullptr,
			1, &image_memory_barrier
		);
	}
	
	void CommandPool::transition_host_image_to_general_layout(Image & image)
	{
		assert__(cmd_buf, "start_submit has not been called");

		VkImageMemoryBarrier image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		image_memory_barrier.srcAccessMask = 0;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex = image_memory_barrier.dstQueueFamilyIndex = device.get_general_queue().get_family_index();
		image_memory_barrier.image = image.get_handle();

		image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_memory_barrier.subresourceRange.baseMipLevel = 0;
		image_memory_barrier.subresourceRange.levelCount = 1;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			cmd_buf,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // source stage
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // destination stage
			0,
			0, nullptr,
			0, nullptr,
			1, &image_memory_barrier
		);
	}

	void CommandPool::wait_for_colour_write(Image const& image)
	{
		assert__(cmd_buf, "start_submit has not been called");
		assert__(image.get_format() != WGI::ImageFormat::DepthF32, "This is a depth image");

		VkImageMemoryBarrier image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
		image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		image_memory_barrier.oldLayout = image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_memory_barrier.srcQueueFamilyIndex = image_memory_barrier.dstQueueFamilyIndex = device.get_general_queue().get_family_index();
		image_memory_barrier.image = image.get_handle();

		image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_memory_barrier.subresourceRange.baseMipLevel = 0;
		image_memory_barrier.subresourceRange.levelCount = 1;
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			cmd_buf,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // source stage
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // destination stage
			0,
			0, nullptr,
			0, nullptr,
			1, &image_memory_barrier
		);

	}

	void CommandPool::start_render_pass(RenderPass const& render_pass, Framebuffer const& framebuffer, bool using_secondary_cmd_buffers)
	{
		assert__(cmd_buf, "start_submit has not been called");

		VkRenderPassBeginInfo begin{};
		begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		begin.renderPass = render_pass.get_handle();
		begin.framebuffer = framebuffer.get_handle();
		begin.renderArea.offset = { 0, 0 };
		auto const& image = *framebuffer.get_any_image_view()->get_image().get();
		begin.renderArea.extent = VkExtent2D{ image.get_width(), image.get_height() };

 		vector<VkClearValue> clear_values;

		if(render_pass.has_colour_image()) {
			clear_values.resize(1);
			clear_values[0].color.float32[0] = 0.0f;
			clear_values[0].color.float32[1] = 1.0f;
			clear_values[0].color.float32[2] = 0.5f;
			clear_values[0].color.float32[3] = 1.0f;
		}


 		// Depth
		if(render_pass.has_depth_image()) {
			VkClearValue clear_value{};
			clear_value.color.float32[0] = 1.0f;
			clear_value.color.float32[1] = 0.0f;
			clear_value.color.float32[2] = 0.0f;
			clear_value.color.float32[3] = 0.0f;
			clear_values.push_back(clear_value);
		}

		begin.clearValueCount = clear_values.size();
		begin.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(cmd_buf, &begin, using_secondary_cmd_buffers ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);
	}


	void CommandPool::bind_pipeline(Ref<Pipeline> pipeline_)
	{
		assert__(cmd_buf, "start_submit has not been called");

		pipeline = move(pipeline_);
		vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_handle());
	}

	void CommandPool::bind_vertex_buffers(Buffer const& buffer, ArrayPointer<const uint64_t> offsets)
	{
		assert__(buffer.has_memory(), "Buffer does not have any memory assigned");
		assert__(offsets.size() && offsets.size() <= 8, "Invalid number of bindings");

		thread_local static array<VkBuffer, 8> buffers;
		for (unsigned int i = 0; i < offsets.size(); i++) {
			assert__(offsets[i] < buffer.get_size(), "Invalid offset");
			buffers[i] = buffer.get_handle();
		}

		vkCmdBindVertexBuffers(cmd_buf, 0, offsets.size(), buffers.data(), offsets.data());
	}

	void CommandPool::bind_index_buffers(Buffer const& buffer, uint64_t offset, bool big_indices)
	{
		assert__(buffer.has_memory(), "Buffer does not have any memory assigned");
		assert__(offset < buffer.get_size(), "Invalid offset");


		vkCmdBindIndexBuffer(cmd_buf, buffer.get_handle(), offset, big_indices ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
	}

	// TODO should this take a Ref<>?
	void CommandPool::bind_descriptor_set(DescriptorPool const& desc, uint32_t i)
	{
		assert__(pipeline, "No pipeline bound");
		VkDescriptorSet set = desc.get_set(i);
		vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_layout(), 0, 1, &set, 0, nullptr);
	}

	void CommandPool::set_push_constants_(optional<ArrayPointer<const uint8_t>> push_constant_data) {
		assert__(pipeline, "No pipeline bound");
		auto pc_sz = pipeline->get_push_constants_size();		
		assert_((!pc_sz and !push_constant_data.has_value()) or 
			(push_constant_data.has_value() and push_constant_data.value().size() == pc_sz));

		if(push_constant_data.has_value()) {
			vkCmdPushConstants(cmd_buf, pipeline->get_layout(), VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
				0, pc_sz,
				push_constant_data.value().data());
		}
	}

	void CommandPool::draw(unsigned int vertices, optional<ArrayPointer<const uint8_t>> push_constant_data)
	{
		set_push_constants_(push_constant_data);
		vkCmdDraw(cmd_buf, vertices, 1, 0, 0);
	}

	void CommandPool::draw_indexed(unsigned int indices, unsigned int start_index, 
		optional<ArrayPointer<const uint8_t>> push_constant_data)
	{
		set_push_constants_(push_constant_data);
		vkCmdDrawIndexed(cmd_buf, indices, 1, start_index, 0, 0);
	}

	void CommandPool::end_render_pass()
	{
		assert__(cmd_buf, "start_submit has not been called before end_render_pass");
		vkCmdEndRenderPass(cmd_buf);
	}

	void CommandPool::transfer_buffer(Buffer const& src, unsigned int src_offset, Buffer const& dst, unsigned int dst_offset, unsigned int size)
	{
		assert__(src.get_usages().transfer_src, "Source buffer is not a valid transfer source");
		assert__(dst.get_usages().transfer_dst, "Destination buffer is not a valid transfer destination");

		VkBufferCopy region{};
		region.srcOffset = src_offset;
		region.dstOffset = dst_offset;
		region.size = size;
		vkCmdCopyBuffer(cmd_buf, src.get_handle(), dst.get_handle(), 1, &region);
	}

	void CommandPool::end_secondary()
	{
		pipeline = nullptr;
		assert_(secondary);		
		assert__(vkEndCommandBuffer(cmd_buf) == VK_SUCCESS, "vkEndCommandBuffer error");
	}

	void CommandPool::submit_and_present(Swapchain const& swapchain, Fence const& fence)
	{
		pipeline = nullptr;

		assert__(cmd_buf, "start_submit has not been called before end_submit");
		assert__(vkEndCommandBuffer(cmd_buf) == VK_SUCCESS, "vkEndCommandBuffer error");
		assert_(!secondary);


		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore sem = swapchain.get_image_available_semaphore();
		VkSemaphore sem2 = swapchain.get_render_complete_semaphore();
		VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &sem;
		submit.pWaitDstStageMask = &stage;
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &sem2;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd_buf;


		assert__(vkQueueSubmit(queue_handle, 1, &submit, fence.get_handle()) == VK_SUCCESS, "vkQueueSubmit error");
	}

	void CommandPool::submit()
	{
		pipeline = nullptr;

		assert__(cmd_buf, "start_submit has not been called before end_submit");
		assert__(vkEndCommandBuffer(cmd_buf) == VK_SUCCESS, "vkEndCommandBuffer error");
		assert_(!secondary);


		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd_buf;


		assert__(vkQueueSubmit(queue_handle, 1, &submit, 
			complete_fence ? complete_fence->get_handle() : VK_NULL_HANDLE) == VK_SUCCESS, "vkQueueSubmit error");
	}

	void CommandPool::execute_secondary(CommandPool& secondary_pool)
	{
		assert_(cmd_buf);
		assert_(!secondary);
		assert_(secondary_pool.secondary);

		auto s = secondary_pool.cmd_buf;

		vkCmdExecuteCommands(cmd_buf, 1, &s);
	}


	// CommandPool::CommandPool(CommandPool&& o) noexcept
	// {
	// 	device = move(o.device);
	// 	command_pool = o.command_pool;
	// 	cmd_buf = o.cmd_buf;
	// 	queue_family_index = o.queue_family_index;
	// 	queue_handle = o.queue_handle;
	// 	secondary = o.secondary;
	// 	pipeline = o.pipeline;
	// 	o.device = nullptr;
	// 	o.command_pool = nullptr;
	// 	o.cmd_buf = nullptr;
	// 	o.queue_handle = nullptr;
	// 	o.pipeline = nullptr;
	// }

	// CommandPool& CommandPool::operator=(CommandPool&& o) noexcept
	// {
	// 	device = move(o.device);
	// 	command_pool = o.command_pool;
	// 	cmd_buf = o.cmd_buf;
	// 	queue_family_index = o.queue_family_index;
	// 	queue_handle = o.queue_handle;
	// 	secondary = o.secondary;
	// 	o.device = nullptr;
	// 	o.command_pool = nullptr;
	// 	o.cmd_buf = nullptr;
	// 	o.queue_handle = nullptr;
	// 	return *this;
	// }
}