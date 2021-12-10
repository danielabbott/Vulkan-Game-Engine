#include "Device.h"
#include "AutoBuffer.h"
#include "CommandPool.h"
#include "Fence.h"

using namespace std;

namespace Vulkan {

	// Transfer usages ignored
	AutoBuffer::AutoBuffer(Device& d, uint32_t bytes, BufferUsages buffer_usages, bool no_device_local)
		:device(d)
	{
		bool device_local_possible = !no_device_local;

		BufferUsages host_visible_buffer_usages = buffer_usages;
		host_visible_buffer_usages.transfer_dst = false;
		host_visible_buffer_usages.transfer_src = false;

		if(device_local_possible) {
			// Buffer in device local memory
			WGI::MemoryCriteria memory_criteria;
			memory_criteria.device_local = WGI::MemoryTypePerference::Must;
			memory_criteria.host_visible = WGI::MemoryTypePerference::MustNot;
			memory_criteria.host_cached = WGI::MemoryTypePerference::MustNot;
			memory_criteria.host_coherent = WGI::MemoryTypePerference::MustNot;

			buffer_usages.transfer_dst = true;
			buffer_usages.transfer_src = false;

			device_local_possible = device.vulkan_memory_type_exists(memory_criteria);

			if(device_local_possible) {
				host_visible_buffer_usages.transfer_src = true;

				device_local_buffer = make_unique<Buffer>(device, bytes, memory_criteria, buffer_usages);
				device_local_memory = Ref<MemoryAllocation>::make(device, memory_criteria);
				device_local_buffer->bind_memory(device_local_memory, WGI::MemoryRegion(0, memory_criteria.size));
			}
		}

		WGI::MemoryCriteria host_visible_memory_criteria;
		host_visible_memory_criteria.device_local = WGI::MemoryTypePerference::PreferedNot;
		host_visible_memory_criteria.host_visible = WGI::MemoryTypePerference::Must;
		host_visible_memory_criteria.host_cached = WGI::MemoryTypePerference::PreferedNot;
		host_visible_memory_criteria.host_coherent = WGI::MemoryTypePerference::Prefered;

		host_visible_buffer = make_unique<Buffer>(device, bytes, host_visible_memory_criteria, host_visible_buffer_usages);


		host_visible_memory = Ref<MemoryAllocation>::make(d, host_visible_memory_criteria);
		host_visible_buffer->bind_memory(host_visible_memory, WGI::MemoryRegion(0, host_visible_memory_criteria.size));
	}

	ArrayPointer<uint8_t> AutoBuffer::map_for_staging()
	{
		return host_visible_memory->map();
	}


	void AutoBuffer::staging_complete(CommandPool& pool, bool async)
	{
		waiting_for_data = false;
		host_visible_memory->flush();
		host_visible_memory->unmap();

		if (!device_local_buffer) {
			return;
		}

		if (async) {
			transfer_complete_fence = make_unique<Fence>(device, false);
		}

		// pool.start_submit();
		pool.transfer_buffer(*host_visible_buffer, 0, *device_local_buffer, 0, host_visible_buffer->get_size());
		// pool.submit(transfer_complete_fence.get());


		// TODO provide a semaphore or memory barrier for non-async transfers


	}


	bool AutoBuffer::can_render()
	{
		if (!device_local_buffer) {
			return true;
		}
		if (!transfer_complete_fence) {
			return true;
		}

		bool done = transfer_complete_fence->poll();
		if (done) {
			transfer_complete_fence = nullptr;
			host_visible_buffer = nullptr;
			host_visible_memory->unmap();
			host_visible_memory = nullptr;
		}
		return done;
	}


	Buffer& AutoBuffer::get_buffer()
	{
		if (device_local_buffer && !transfer_complete_fence) {
			return *device_local_buffer;
		}
		return *host_visible_buffer;
	}


}