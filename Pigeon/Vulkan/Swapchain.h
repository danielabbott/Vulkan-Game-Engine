#pragma once
#include "Device.h"
#include "ImageView.h"
#include "Framebuffer.h"
#include "RenderPass.h"
#include <vector>
#include "../Ref.h"
#include <optional>
#include <stdexcept>
#include "Fence.h"
#include <memory>

struct VkSwapchainKHR_T;
struct VkSemaphore_T;

namespace Vulkan {
	static inline WGI::ImageFormat get_swapchain_format() { return WGI::ImageFormat::SRGBU8; }

	class Swapchain {
		Device& device;
		VkSwapchainKHR_T* swapchain = nullptr;
		std::vector<Ref<Image>> swapchain_images;
		std::vector<Ref<ImageView>> swapchain_image_views;
		unsigned int width = 0, height = 0;
		std::vector<VkSemaphore_T*> image_available_semaphores;
		std::unique_ptr<Fence> image_available_fence;
		std::vector<VkSemaphore_T*> render_finished_semaphores;
		std::vector<Fence> render_finished_fences;

		// The swapchain image being used this frame. Used for framebuffers etc.
		uint32_t image_index = -1;

		// Alternates between 0 and 1 (for 2 swapchain images). Used for semaphores etc.
		uint32_t frame_number = 0;

		// bool try_next_image_called = false;
	public:


		// Render pass is the last pass which writes to the swapchain
		Swapchain(Device&);
		~Swapchain();

		Device& get_device() const;

		// If width or height is 0 then the window is not visible (probably minimised)
		unsigned int get_width() const;
		unsigned int get_height() const;

		unsigned int get_number_of_images() const { return static_cast<unsigned int>(swapchain_images.size()); }

		uint32_t next_image();
		//std::optional<uint32_t> try_next_image(uint32_t frame_number_);

		Ref<ImageView> const& get_image_view(unsigned int i) const { return swapchain_image_views[i]; }

		VkSemaphore_T* get_image_available_semaphore() const;
		VkSemaphore_T* get_render_complete_semaphore() const;
		Fence const& get_render_complete_fence() const;



		void present();
		void wait_for_render_finish();


		Swapchain(const Swapchain&) = delete;
		Swapchain& operator=(const Swapchain&) = delete;
		Swapchain(Swapchain&&) = delete;
		Swapchain& operator=(Swapchain&&) = delete;
	};

	class RecreateSwapchainException : std::exception {

	};

}