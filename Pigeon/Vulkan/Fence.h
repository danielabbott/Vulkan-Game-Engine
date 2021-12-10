#pragma once

#include "../Ref.h"

struct VkFence_T;

namespace Vulkan {

	class Device;

	class Fence {
		std::reference_wrapper<Device> device;
		VkFence_T* fence = nullptr;

		void do_move(Fence && other) noexcept;
	public:
		Fence(Device&, bool start_triggered);
		~Fence();

		bool poll() const;
		void wait() const;
		void reset() const;

		VkFence_T* get_handle() const {
			return fence;
		}

		Fence(const Fence&) = delete;
		Fence& operator=(const Fence&) = delete;
		Fence(Fence&&) noexcept;
		Fence& operator=(Fence&&) noexcept;
	};
}
