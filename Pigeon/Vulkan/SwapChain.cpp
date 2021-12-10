#define INCLUDE_GLFW
#include "Include.h"

#include "Swapchain.h"
#include "Device.h"
#include "Device.h"
#include "../WGI/Window.h"

#include "../IncludeSpdlog.h"
#include "../DebugTimer.h"

using namespace std;

namespace Vulkan {

	Swapchain::Swapchain(Device& device_)
		: device(device_)
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.get_physical_device_handle_(),
			device.get_surface_handle_(), &capabilities);


		unsigned int image_count = 2;
		if (capabilities.maxImageCount == 1) {
			image_count = 1;
		}
		if (capabilities.minImageCount > 2) {
			image_count = capabilities.minImageCount;
		}

		VkSwapchainCreateInfoKHR create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = device.get_surface_handle_();
		create_info.minImageCount = image_count;
		create_info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
		create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		glfwGetFramebufferSize(device.get_window()->get_handle(),
			reinterpret_cast<int*>(&width), reinterpret_cast<int*>(&height));
		create_info.imageExtent.width = width;
		create_info.imageExtent.height = height;
		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		create_info.preTransform = capabilities.currentTransform;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		create_info.clipped = VK_TRUE;

		// If old swapchain is full-screen
		//create_info.oldSwapchain = swapchain;

		assert__(vkCreateSwapchainKHR(device.get_device_handle_(), &create_info, nullptr, &swapchain) == VK_SUCCESS, "Error creating swapchain");

		vkGetSwapchainImagesKHR(device.get_device_handle_(), swapchain, &image_count, nullptr);
		vector<VkImage> images(image_count);
		swapchain_images.reserve(image_count);
		vkGetSwapchainImagesKHR(device.get_device_handle_(), swapchain, &image_count, images.data());

		for (VkImage img : images) {
			swapchain_images.push_back(Ref<Image>::make(device, img, WGI::ImageFormat::SRGBU8, width, height));
			swapchain_image_views.push_back(Ref<ImageView>::make(swapchain_images[swapchain_images.size() - 1]));
		}

		VkSemaphoreCreateInfo semaphore_create{};
		semaphore_create.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


		for (unsigned int i = 0; i < image_count; i++) {
			VkSemaphore sem;
			assert__(vkCreateSemaphore(device.get_device_handle_(), &semaphore_create, nullptr, &sem) == VK_SUCCESS,
				"vkCreateSemaphore error");
			image_available_semaphores.push_back(sem);


			assert__(vkCreateSemaphore(device.get_device_handle_(), &semaphore_create, nullptr, &sem) == VK_SUCCESS,
				"vkCreateSemaphore error");
			render_finished_semaphores.push_back(sem);


			render_finished_fences.emplace_back(device, true);
		}


		image_available_fence = make_unique<Fence>(device, false);
	}

	uint32_t Swapchain::next_image()
	{
		frame_number = (frame_number+1) % swapchain_images.size();

		render_finished_fences[frame_number].wait();
		render_finished_fences[frame_number].reset();

		auto d = device.get_device_handle_();

		VkResult result = vkAcquireNextImageKHR(d, swapchain, UINT64_MAX, get_image_available_semaphore(), image_available_fence->get_handle(), &image_index);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			throw RecreateSwapchainException();
		}

		// Accept VK_SUBOPTIMAL_KHR as it is a success code. Exception will be thrown when image is presented.
		assert__((result == VK_SUCCESS or result == VK_SUBOPTIMAL_KHR) and image_index < 999, "vkAcquireNextImageKHR error");

		image_available_fence->wait();
		image_available_fence->reset();

		return image_index;
	}


	VkSemaphore_T* Swapchain::get_image_available_semaphore() const {
		return image_available_semaphores[frame_number];
	}
	VkSemaphore_T* Swapchain::get_render_complete_semaphore() const {
		return render_finished_semaphores[frame_number];
	}
	Fence const& Swapchain::get_render_complete_fence() const {
		return render_finished_fences[frame_number];
	}

	Swapchain::~Swapchain()
	{
		auto d = device.get_device_handle_();

		// swapchain_framebuffers.clear();
		// swapchain_image_views.clear();
		// swapchain_images.clear();
		// render_finished_fences.clear();

		for (unsigned int i = 0; i < image_available_semaphores.size(); i++) {
			vkDestroySemaphore(d, image_available_semaphores[i], nullptr);
			vkDestroySemaphore(d, render_finished_semaphores[i], nullptr);
		}

		image_available_fence = nullptr;
		vkDestroySwapchainKHR(device.get_device_handle_(), swapchain, nullptr);
	}

	Device& Swapchain::get_device() const
	{
		return device;
	}

	unsigned int Swapchain::get_width() const
	{
		return width;
	}

	unsigned int Swapchain::get_height() const
	{
		return height;
	}

	void Swapchain::present()
	{
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

		VkSemaphore sem = get_render_complete_semaphore();
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &sem;

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &image_index;

		VkResult result = vkQueuePresentKHR(device.get_general_queue().get_queue_handle(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			throw RecreateSwapchainException();
		}

		assert__(result == VK_SUCCESS, "vkQueuePresentKHR error");
	}

}