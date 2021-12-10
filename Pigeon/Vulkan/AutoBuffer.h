// Handles creation of staging buffer for dedicated GPUs

#pragma once

#include "Buffer.h"
#include "../Ref.h"
#include "../ArrayPointer.h"
#include "Fence.h"
#include "Memory.h"
#include <memory>

namespace Vulkan {

	class Device;
	class CommandPool;

	// TODO use an allocator instead of doing dedicated allocations

	class AutoBuffer {
		Device& device;

		std::unique_ptr<Buffer> device_local_buffer = nullptr;
		Ref<MemoryAllocation> device_local_memory = nullptr;

		std::unique_ptr<Buffer> host_visible_buffer = nullptr;
		Ref<MemoryAllocation> host_visible_memory = nullptr;


		bool waiting_for_data = true;
		std::unique_ptr<Fence> transfer_complete_fence = nullptr;

	public:
		// Transfer usages ignored
		// If no_device_local is true then this is a host-only buffer. Used for uniform buffers and other dynamic data
		AutoBuffer(Device&, uint32_t bytes, BufferUsages, bool no_device_local = false);

		ArrayPointer<uint8_t> map_for_staging();

		// unmaps and starts transfer
		void staging_complete(CommandPool&, bool async);


		// Poll this each frame until the transfer has completed
		bool can_render();

		// Gets a buffer to render from
		// Only call this once can_render() returns true
		Buffer& get_buffer();
	};
}