#pragma once

#include "../Ref.h"
#include <optional>
#include "Buffer.h"
#include "Memory.h"
#include <memory>

struct VkCommandPool_T;
struct VkCommandBuffer_T;
struct VkFence_T;


namespace Vulkan {

	class RenderPass;
	class Framebuffer;
	class Pipeline;
	class Swapchain;
	class Device;
	class Queue;
	class Fence;
	class DescriptorPool;
	class Image;
	class QueryPool;

	class CommandPool {
		Device& device;
		VkCommandPool_T* command_pool = nullptr;
		VkCommandBuffer_T* cmd_buf = nullptr;
		unsigned int queue_family_index;
		VkQueue_T* queue_handle = nullptr;
		std::unique_ptr<Fence> complete_fence;
		bool secondary;
		Ref<Pipeline> pipeline;

		void set_push_constants_(std::optional<ArrayPointer<const uint8_t>> push_constant_data);
	public:
		CommandPool(Device&, Queue const&, bool create_fence, bool secondary);
		~CommandPool();

		// render_pass can be nullptr for primary command buffers
		// If swapchain is not nullptr then the viewport and scissor state will be set
		void start_submit(std::optional<std::reference_wrapper<const RenderPass>>,
			std::optional<std::reference_wrapper<const Framebuffer>>);

		void set_viewport_size(unsigned int w, unsigned int h);

		void reset_query_pool(QueryPool&);
		void set_timer(QueryPool&, unsigned int n);

		void wait_for_depth_write(Image const&);
		void wait_for_colour_write(Image const&);

		// For textures in host memory. Assumes image is in preinitialised layout
		void transition_host_image_to_general_layout(Image & image);


		// Render pass and framebuffer need to stay valid until the command completes, obviously.

		void start_render_pass(RenderPass const&, Framebuffer const&, bool using_secondary_cmd_buffers);
		void bind_pipeline(Ref<Pipeline>); // Holds pointer until end_secondary, submit, or submit_and_present
		void bind_vertex_buffers(Buffer const& buffer, ArrayPointer<const uint64_t> offsets);
		void bind_index_buffers(Buffer const& buffer, uint64_t offset, bool big_indices);
		void bind_descriptor_set(DescriptorPool const&, uint32_t i);
		void draw(unsigned int vertices, std::optional<ArrayPointer<const uint8_t>> push_constant_data);
		void draw_indexed(unsigned int indices, unsigned int start_index = 0, std::optional<ArrayPointer<const uint8_t>> push_constant_data = std::nullopt);
		void end_render_pass();

		// Call when finished drawing/uploading on secondary command buffer
		void end_secondary();

		void execute_secondary(CommandPool& secondary);

		void transfer_buffer(Buffer const& src, unsigned int src_offset, Buffer const& dst, unsigned int dst_offset, unsigned int size);

		void submit_and_present(Swapchain const&, Fence const&);

		void submit();

		bool is_complete();
		void wait() const;

		CommandPool(const CommandPool&) = delete;
		CommandPool& operator=(const CommandPool&) = delete;
		// CommandPool(CommandPool&&) noexcept;
		// CommandPool& operator=(CommandPool&&) noexcept;
		CommandPool(CommandPool&&) = delete;
		CommandPool& operator=(CommandPool&&) = delete;
	};
}