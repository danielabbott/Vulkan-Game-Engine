#include "Device.h"
#include "Buffer.h"
#include "AutoBuffer.h"
#include "ImageView.h"
#include "DescriptorSets.h"
#include "../BetterAssert.h"
#include "Include.h"
#include "Sampler.h"

using namespace std;

namespace Vulkan {

	DescriptorLayout::DescriptorLayout(Device& d, ArrayPointer<DescriptorType> d_types)
		:device(d), number_of_bindings(d_types.size())
	{
		vector<VkDescriptorSetLayoutBinding> layouts;
		layouts.resize(d_types.size());

		for(unsigned int i = 0; i < d_types.size(); i++) {
			if(d_types[i].type == DescriptorType::Type::Uniform) {
				number_of_uniforms += d_types[i].array_elements;
				layouts[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			}
			else if(d_types[i].type == DescriptorType::Type::Texture) {
				number_of_textures += d_types[i].array_elements;
				layouts[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			}
			else {
				assert_(false);
			}

			assert_(d_types[i].array_elements);
			layouts[i].descriptorCount = d_types[i].array_elements;
			layouts[i].stageFlags = 0;
			if(d_types[i].vertex_shader_accessible) {
				layouts[i].stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			} 
			if(d_types[i].fragment_shader_accessible) {
				layouts[i].stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			}
			layouts[i].binding = i;
		}

		VkDescriptorSetLayoutCreateInfo layout{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layout.bindingCount = layouts.size();
		layout.pBindings = layouts.data();

		assert__(vkCreateDescriptorSetLayout(device.get_device_handle_(), &layout, nullptr, &descriptor_set_layout) == VK_SUCCESS,
			"vkCreateDescriptorSetLayout error");

	}
	
	DescriptorLayout::~DescriptorLayout()
	{
		vkDestroyDescriptorSetLayout(device.get_device_handle_(), descriptor_set_layout, nullptr);		
	}

	DescriptorPool::DescriptorPool(Device& d, ArrayPointer<const reference_wrapper<const DescriptorLayout>> descriptor_layouts)
		: device(d)
	{
		unsigned int uniform_descriptors_total = 0;
		unsigned int texture_descriptors_total = 0;
		for(unsigned int i = 0; i < descriptor_layouts.size(); i++) {
			uniform_descriptors_total += descriptor_layouts[i].get().get_number_of_uniforms();
			texture_descriptors_total += descriptor_layouts[i].get().get_number_of_textures();
		}

		vector<VkDescriptorPoolSize> pool_sizes;

		if(uniform_descriptors_total) {
			VkDescriptorPoolSize pool_size{};
			pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			pool_size.descriptorCount = uniform_descriptors_total;
			pool_sizes.push_back(pool_size);
		}

		if(texture_descriptors_total) {
			VkDescriptorPoolSize pool_size{};
			pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			pool_size.descriptorCount = texture_descriptors_total;
			pool_sizes.push_back(pool_size);
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = pool_sizes.size();
		poolInfo.pPoolSizes = pool_sizes.data();
		poolInfo.maxSets = descriptor_layouts.size();

		assert__(vkCreateDescriptorPool(device.get_device_handle_(), &poolInfo, nullptr, &pool) == VK_SUCCESS, "vkCreateDescriptorPool error");


		vector<VkDescriptorSetLayout> layouts;
		layouts.resize(descriptor_layouts.size());
		for(unsigned int i = 0; i < descriptor_layouts.size(); i++) {
			layouts[i] = descriptor_layouts[i].get().get_handle_();
		}

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = pool;
		allocInfo.descriptorSetCount = descriptor_layouts.size();
		allocInfo.pSetLayouts = layouts.data();


		descriptor_sets.resize(descriptor_layouts.size());
		buffers.resize(descriptor_layouts.size());
		assert__(vkAllocateDescriptorSets(device.get_device_handle_(), &allocInfo, descriptor_sets.data()) == VK_SUCCESS, "vkAllocateDescriptorSets error");
	

		buffers.resize(descriptor_layouts.size());
		auto_buffers.resize(descriptor_layouts.size());
		image_views.resize(descriptor_layouts.size());
		samplers.resize(descriptor_layouts.size());

		for(unsigned int i = 0; i < descriptor_layouts.size(); i++) {
			unsigned int n = descriptor_layouts[i].get().get_number_of_bindings();
			buffers[i].resize(n);
			auto_buffers[i].resize(n);

			image_views[i].resize(n);
			samplers[i].resize(n);
		}

	}

	void DescriptorPool::update_descriptor(unsigned int set, unsigned int binding, Ref<Buffer> buffer)
	{
		assert_(buffer);

		if(buffers[set][binding] == buffer) {
			return;
		}

		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = buffer->get_handle();
		buffer_info.offset = 0;
		buffer_info.range = buffer->get_size();

		VkWriteDescriptorSet descriptor_write{};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstSet = descriptor_sets[set];
		descriptor_write.dstBinding = binding;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pBufferInfo = &buffer_info;
		vkUpdateDescriptorSets(device.get_device_handle_(), 1, &descriptor_write, 0, nullptr);

		buffers[set][binding] = buffer;
		auto_buffers[set][binding] = nullptr;

		auto& v1 = image_views[set][binding];
		fill(v1.begin(), v1.end(), nullptr);

		auto& v2 = samplers[set][binding];
		fill(v2.begin(), v2.end(), nullptr);
	}

	void DescriptorPool::update_descriptor(unsigned int set, unsigned int binding, Ref<AutoBuffer> buffer)
	{
		assert_(buffer);

		if(auto_buffers[set][binding] == buffer) {
			return;
		}

		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = buffer->get_buffer().get_handle();
		buffer_info.offset = 0;
		buffer_info.range = buffer->get_buffer().get_size();

		VkWriteDescriptorSet descriptor_write{};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstSet = descriptor_sets[set];
		descriptor_write.dstBinding = binding;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pBufferInfo = &buffer_info;
		vkUpdateDescriptorSets(device.get_device_handle_(), 1, &descriptor_write, 0, nullptr);

		buffers[set][binding] = nullptr;
		auto_buffers[set][binding] = move(buffer);

		auto& v1 = image_views[set][binding];
		fill(v1.begin(), v1.end(), nullptr);

		auto& v2 = samplers[set][binding];
		fill(v2.begin(), v2.end(), nullptr);
	}
	
	void DescriptorPool::update_descriptor(unsigned int set, unsigned int binding, Ref<ImageView> image, Ref<Sampler> sampler, unsigned int sampler_array_index)
	{
		assert_(image);
		assert_(sampler);

		{
			auto& v1 = image_views[set][binding];
			if(v1.size() <= sampler_array_index) {
				v1.resize(sampler_array_index+1);
			}

			auto& v2 = samplers[set][binding];
			if(v2.size() <= sampler_array_index) {
				v2.resize(sampler_array_index+1);
			}

			if(v1[sampler_array_index] == image and v2[sampler_array_index] == sampler) {
				return;
			}
		}


		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = 
			image->get_image()->get_format() == WGI::ImageFormat::DepthF32 ? 
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		image_info.imageView = image->get_handle();
		image_info.sampler = sampler->get_handle_();

		VkWriteDescriptorSet descriptor_write{};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstSet = descriptor_sets[set];
		descriptor_write.dstBinding = binding;
		descriptor_write.dstArrayElement = sampler_array_index;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pImageInfo = &image_info;
		vkUpdateDescriptorSets(device.get_device_handle_(), 1, &descriptor_write, 0, nullptr);

		buffers[set][binding] = nullptr;
		auto_buffers[set][binding] = nullptr;
		image_views[set][binding][sampler_array_index] = move(image);
		samplers[set][binding][sampler_array_index] = move(sampler);
	}

	DescriptorPool::~DescriptorPool() {
		vkDestroyDescriptorPool(device.get_device_handle_(), pool, nullptr);
	}
}
