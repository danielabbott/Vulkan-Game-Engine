#include "Device.h"
#include "Fence.h"
#include "../BetterAssert.h"
#include "Include.h"

using namespace std;


namespace Vulkan {

	Fence::Fence(Device& d, bool start_triggered)
		:device(d)
	{
		VkFenceCreateInfo fence_create{};
		fence_create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_create.flags = start_triggered ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

		assert__(vkCreateFence(device.get().get_device_handle_(), &fence_create, nullptr, &fence) == VK_SUCCESS,
			"vkCreateFence error");
	}

	Fence::~Fence()
	{
		vkDestroyFence(device.get().get_device_handle_(), fence, nullptr);
	}


	bool Fence::poll() const
	{
		auto status = vkGetFenceStatus(device.get().get_device_handle_(), fence);
		assert__(status == VK_SUCCESS || status == VK_NOT_READY, "vkGetFenceStatus error");
		return status == VK_SUCCESS;
	}

	void Fence::wait() const
	{
		assert__(vkWaitForFences(device.get().get_device_handle_(), 1, &fence, VK_TRUE, UINT64_MAX) == VK_SUCCESS, "vkWaitForFences error");
	}

	void Fence::reset() const
	{
		vkResetFences(device.get().get_device_handle_(), 1, &fence);
	}

	void Fence::do_move(Fence && other) noexcept
	{
		fence = other.fence;
		other.fence = VK_NULL_HANDLE;
	}

	Fence::Fence(Fence&& other) noexcept
	: device(other.device)
	{
		do_move(move(other));
	}

	Fence& Fence::operator=(Fence&& other) noexcept
	{
		device = other.device;
		do_move(move(other));
		return *this;
	}
}