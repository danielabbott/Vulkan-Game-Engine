#pragma once

#include "ImageView.h"
#include "RenderPass.h"

struct VkFramebuffer_T;

namespace Vulkan {

	class Framebuffer
	{
		Ref<ImageView> image_view;
		Ref<ImageView> depth_image_view;
		VkFramebuffer_T* handle = nullptr;
	public:
		// Framebuffer must only be used with render passes compatible with the render pass given
		Framebuffer(RenderPass const&, Ref<ImageView> colour, Ref<ImageView> depth);
		~Framebuffer();

		VkFramebuffer_T* get_handle() const { return handle; }
		Ref<ImageView> const& get_image_view() const { return image_view; }
		Ref<ImageView> const& get_depth_image_view() const { return depth_image_view; }
		Ref<ImageView> const& get_any_image_view() const { return image_view ? image_view : depth_image_view; }

		Framebuffer(const Framebuffer&) = delete;
		Framebuffer& operator=(const Framebuffer&) = delete;
		Framebuffer(Framebuffer&&) = delete;
		Framebuffer& operator=(Framebuffer&&) = delete;
	};

}