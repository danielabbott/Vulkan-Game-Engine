#pragma once

#include <stdbool.h>

struct VkSampler_T;

typedef struct PigeonVulkanSampler {
	struct VkSampler_T* vk_sampler;
} PigeonVulkanSampler;

int pigeon_vulkan_create_sampler(PigeonVulkanSampler*, bool linear_filter, bool anisiotropic_filtering,
	bool shadow_map, bool clamp_to_edge, bool multi_mip_map_sample);
void pigeon_vulkan_destroy_sampler(PigeonVulkanSampler*);
