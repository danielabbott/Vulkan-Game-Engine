#pragma once

#include <vector>
#include "../Ref.h"
#include "Device.h"
#include "Image.h"

struct VkRenderPass_T;

namespace Vulkan {

	class Image;

	

	class RenderPass
	{
		Device& device;
		VkRenderPass_T* render_pass = nullptr;

		bool has_colour_image_;
		bool has_depth_image_;

	public:
		enum class DepthMode {
			None, // No depth buffer
			ReadOnly, // Read-only depth buffer provided for depth testing
			Discard, // Depth buffer will be written to for depth testing then discarded
			Keep // Depth buffer will be written to for depth testing and data will be available after
		};

		struct CreateInfo {
			bool depends_on_transfer = true;

			DepthMode depth = DepthMode::None;

			std::optional<WGI::ImageFormat> colour_image = std::nullopt;
			bool colour_image_is_swapchain = false; // used if colour_image != nullopt
			bool clear_colour_image = true; // false = ignore previous contents of framebuffer
		};

		RenderPass(Device&, CreateInfo info);
		~RenderPass();

		Device& get_device() const { return device; }
		VkRenderPass_T* get_handle() const { return render_pass; }

		bool has_colour_image() const { return has_colour_image_; }
		bool has_depth_image() const { return has_depth_image_; }


		RenderPass(const RenderPass&) = delete;
		RenderPass& operator=(const RenderPass&) = delete;
		RenderPass(RenderPass&&) = delete;
		RenderPass& operator=(RenderPass&&) = delete;
	};

}