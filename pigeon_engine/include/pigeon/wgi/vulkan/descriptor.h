#pragma once

#include <stdbool.h>
#include "image.h"
#include "sampler.h"
#include "buffer.h"

/*

Binding: uniform Sampler2D my_images[3];
Set: All bindings used by a shader program

A layout describes a set.

A pool holds descriptors for many sets.
A descriptor defines a binding.

*/

struct VkDescriptorSetLayout_T;
struct VkDescriptorPool_T;
struct VkDescriptorSet_T;

typedef enum {
	PIGEON_VULKAN_DESCRIPTOR_TYPE_UNIFORM,
	PIGEON_VULKAN_DESCRIPTOR_TYPE_SSBO,
	PIGEON_VULKAN_DESCRIPTOR_TYPE_TEXTURE,
	PIGEON_VULKAN_DESCRIPTOR_TYPE_IMAGE
} PigeonVulkanDescriptorType;

typedef struct PigeonVulkanDescriptorBinding {
	PigeonVulkanDescriptorType type;
	bool vertex_shader_accessible;
	bool fragment_shader_accessible;
	bool compute_shader_accessible;
	unsigned int elements; // Array elements. e.g. uniform sampler2D textures[3];
} PigeonVulkanDescriptorBinding;

typedef struct PigeonVulkanDescriptorLayout {
	struct VkDescriptorSetLayout_T* handle;
	unsigned int number_of_uniforms;
	unsigned int number_of_textures;
	unsigned int number_of_storage_images;
	unsigned int number_of_ssbos;
} PigeonVulkanDescriptorLayout;


typedef struct PigeonVulkanDescriptorPool {
	struct VkDescriptorPool_T* vk_descriptor_pool;
	unsigned int number_of_sets;

	
	union {
		struct VkDescriptorSet_T* vk_descriptor_set; // if number_of_sets = 1
		struct VkDescriptorSet_T** vk_descriptor_sets; // if number_of_sets > 1
	};
} PigeonVulkanDescriptorPool;



int pigeon_vulkan_create_descriptor_layout(PigeonVulkanDescriptorLayout* layout,
	unsigned int bindings_count, PigeonVulkanDescriptorBinding* bindings);
void pigeon_vulkan_destroy_descriptor_layout(PigeonVulkanDescriptorLayout* layout);



// One layout per set
int pigeon_vulkan_create_descriptor_pool(PigeonVulkanDescriptorPool*, 
	unsigned int set_count, PigeonVulkanDescriptorLayout*);

void pigeon_vulkan_set_descriptor_uniform_buffer(PigeonVulkanDescriptorPool*, 
	unsigned int set, unsigned int binding, unsigned int array_index, PigeonVulkanBuffer*);
void pigeon_vulkan_set_descriptor_uniform_buffer2(PigeonVulkanDescriptorPool*, 
	unsigned int set, unsigned int binding, unsigned int array_index, PigeonVulkanBuffer*,
	uint64_t offset, uint64_t range);

void pigeon_vulkan_set_descriptor_ssbo(PigeonVulkanDescriptorPool*, 
	unsigned int set, unsigned int binding, unsigned int array_index, PigeonVulkanBuffer*);
void pigeon_vulkan_set_descriptor_ssbo2(PigeonVulkanDescriptorPool*, 
	unsigned int set, unsigned int binding, unsigned int array_index, PigeonVulkanBuffer*,
	uint64_t offset, uint64_t range);

// Array index: uniform Sampler2D textures[3]; <- index into array
void pigeon_vulkan_set_descriptor_texture(PigeonVulkanDescriptorPool*, 
	unsigned int set, unsigned int binding, unsigned int array_index, PigeonVulkanImageView*, PigeonVulkanSampler*);

void pigeon_vulkan_set_descriptor_storage_image(PigeonVulkanDescriptorPool*, 
	unsigned int set, unsigned int binding, unsigned int array_index, PigeonVulkanImageView*);

void pigeon_vulkan_destroy_descriptor_pool(PigeonVulkanDescriptorPool*);
