#pragma once

#include "../Ref.h"
#include "../WGI/Image.h"
#include "Memory.h"
#include <optional>

struct VkImage_T;

namespace Vulkan {

	class Device;
	

	class Image {
		Device& device;
		VkImage_T* image_handle = nullptr;
		bool free_image_in_destructor = true;
		WGI::ImageFormat format;
		unsigned int width;
		unsigned int height;
		std::optional<Ref<MemoryAllocation>> memory = std::nullopt;
		unsigned int memory_size_requirement;
		VulkanMemoryProperties memory_properties; // valid if memory is valid
		WGI::MemoryRegion memory_region;

	public:
		// image_handle can be nullptr
		Image(Device&, WGI::ImageFormat, unsigned int width, unsigned int height, WGI::MemoryCriteria&, bool host_memory_texture = false);
		Image(Device&, VkImage_T* image_handle, WGI::ImageFormat, unsigned int width, unsigned int height);
		~Image();

		VkImage_T* get_handle() const { return image_handle; }
		WGI::ImageFormat get_format() const { return format; }
		Device& get_device() const { return device; }
		unsigned int get_width() const { return width; }
		unsigned int get_height() const { return height; }
		std::pair<unsigned int, unsigned int> get_dimensions() const { return std::pair<unsigned int, unsigned int>(width, height); }
		// unsigned int get_size() const { return width*height*get_image_format_stride(format); }

		void bind_memory(Ref<MemoryAllocation>, WGI::MemoryRegion);
		bool has_memory() const { return memory.has_value(); }

		MemoryAllocation& get_memory() const { return *memory.value(); }
		WGI::MemoryRegion get_memory_region() const { return memory_region; }

		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;
		Image(Image&&) = delete;
		Image& operator=(Image&&) = delete;
	};
}