#pragma once

#include "../HeapArray.h"
#include "../Ref.h"
#include "../ArrayPointer.h"

struct VkDescriptorPool_T;
struct VkDescriptorSetLayout_T;
struct VkDescriptorSet_T;

namespace Vulkan {
	class Device;
	class Buffer;
	class AutoBuffer;
	class ImageView;
	class Sampler;

	class DescriptorLayout {
		Device& device;
		VkDescriptorSetLayout_T* descriptor_set_layout = nullptr;

		unsigned int number_of_uniforms = 0;
		unsigned int number_of_textures = 0;
		unsigned int number_of_bindings = 0;
	public:
		struct DescriptorType {
			bool vertex_shader_accessible = true;
			bool fragment_shader_accessible = true;

			enum Type {
				Uniform, Texture
				//Storage
			};
			Type type = Type::Uniform;

			unsigned int array_elements = 1;

			DescriptorType() {}
			DescriptorType(Type type_) : type(type_) {}
		};

		DescriptorLayout(Device&, ArrayPointer<DescriptorType>);
		~DescriptorLayout();

		unsigned int get_number_of_uniforms() const { return number_of_uniforms; };
		unsigned int get_number_of_textures() const { return number_of_textures; };
		unsigned int get_number_of_bindings() const { return number_of_bindings; };

		VkDescriptorSetLayout_T* get_handle_() const { return descriptor_set_layout; }
	};

	class DescriptorPool {
		Device& device;
		VkDescriptorPool_T* pool = nullptr;
		std::vector<VkDescriptorSet_T*> descriptor_sets;

		// Outer vector is for descriptor sets, inner vector for descriptors
		std::vector<std::vector<Ref<Buffer>>> buffers;
		std::vector<std::vector<Ref<AutoBuffer>>> auto_buffers;

		// Inner inner vector for array index
		std::vector<std::vector<std::vector<Ref<ImageView>>>> image_views;
		std::vector<std::vector<std::vector<Ref<Sampler>>>> samplers;
	public:

		// One descriptor layout per set
		DescriptorPool(Device&, ArrayPointer<const std::reference_wrapper<const DescriptorLayout>>);
		~DescriptorPool();

		void update_descriptor(unsigned int set, unsigned int binding, Ref<Buffer> buffer);
		void update_descriptor(unsigned int set, unsigned int binding, Ref<AutoBuffer> buffer);
		void update_descriptor(unsigned int set, unsigned int binding, Ref<ImageView> image, Ref<Sampler>, unsigned int sampler_array_index = 0);

		VkDescriptorSetLayout_T* get_layout() const;
		VkDescriptorSet_T* get_set(uint32_t i) const { return descriptor_sets[i]; }


		DescriptorPool(const DescriptorPool&) = delete;
		DescriptorPool& operator=(const DescriptorPool&) = delete;
		DescriptorPool(DescriptorPool&&) = delete;
		DescriptorPool& operator=(DescriptorPool&&) = delete;
	};

}
