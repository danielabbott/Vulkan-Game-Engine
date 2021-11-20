#include "singleton.h"
#include <pigeon/assert.h>
#include <pigeon/wgi/vulkan/sampler.h>

int pigeon_vulkan_create_sampler(PigeonVulkanSampler* sampler, bool linear_filter, bool anisiotropic_filtering,
	bool shadow_map, bool clamp_to_edge, bool multi_mip_map_sample)
{
	ASSERT_R1(sampler);

	VkSamplerCreateInfo sampler_create = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	if (linear_filter) {
		sampler_create.magFilter = VK_FILTER_LINEAR;
		sampler_create.minFilter = VK_FILTER_LINEAR;
	} else {
		sampler_create.magFilter = VK_FILTER_NEAREST;
		sampler_create.minFilter = VK_FILTER_NEAREST;
	}

	if (multi_mip_map_sample) {
		sampler_create.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_create.maxLod = VK_LOD_CLAMP_NONE;
	}

	if (singleton_data.anisotropy_supported && anisiotropic_filtering) {
		sampler_create.anisotropyEnable = VK_TRUE;
		sampler_create.maxAnisotropy = 16;
	}
	sampler_create.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

	if (shadow_map) {
		sampler_create.compareEnable = true;
		sampler_create.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
		sampler_create.magFilter = VK_FILTER_LINEAR;
		sampler_create.minFilter = VK_FILTER_LINEAR;
	}

	if (clamp_to_edge) {
		sampler_create.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_create.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}

	ASSERT_LOG_R1(
		vkCreateSampler(vkdev, &sampler_create, NULL, &sampler->vk_sampler) == VK_SUCCESS, "vkCreateSampler error");

	return 0;
}

void pigeon_vulkan_destroy_sampler(PigeonVulkanSampler* sampler)
{
	if (sampler && sampler->vk_sampler) {
		vkDestroySampler(vkdev, sampler->vk_sampler, NULL);
		sampler->vk_sampler = NULL;
	} else {
		assert(false);
	}
}
