#pragma once

#include "../Ref.h"

#include "Image.h"

struct VkImageView_T;

namespace Vulkan {

	class Image;

	class ImageView {
		Ref< Image> image;
		VkImageView_T* handle = nullptr;
	public:
		ImageView(Ref<Image>);
		~ImageView();

		Ref<Image> const& get_image() const { return image; }
		VkImageView_T* get_handle() const { return handle; }

		ImageView(const ImageView&) = delete;
		ImageView& operator=(const ImageView&) = delete;
		ImageView(ImageView&&) = delete;
		ImageView& operator=(ImageView&&) = delete;
	};
}