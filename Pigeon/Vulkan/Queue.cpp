#include "Queue.h"
#include <optional>
#include "../BetterAssert.h"
#include "Include.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "Pipeline.h"


using namespace std;


namespace Vulkan {

	Queue::Queue(VkDevice device_, VkQueue h, uint32_t family_index_,
			bool graphics_, bool transfer_, bool compute_, unsigned int timestamp_bits_valid_)
		: device(device_), queue_handle(h), family_index(family_index_),
		graphics(graphics_), transfer(transfer_), compute(compute_),
		timestamp_bits_valid(timestamp_bits_valid_)
	{
	}

	Queue::~Queue() {
	}


	//Queue::Queue(Queue&& other) noexcept
	//{
	//	//family = other.family;
	//	//graphics = other.graphics;
	//	//transfer = other.transfer;
	//	other.queue_handle = nullptr;
	//}
	//
	//Queue& Queue::operator=(Queue&& other) noexcept
	//{
	//	//family = other.family;
	//	//graphics = other.graphics;
	//	//transfer = other.transfer;
	//	other.queue_handle = nullptr;
	//	return *this;
	//}
}