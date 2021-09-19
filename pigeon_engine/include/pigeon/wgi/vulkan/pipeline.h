#pragma once

#include <pigeon/wgi/vulkan/renderpass.h>
#include <pigeon/wgi/vulkan/descriptor.h>
#include <pigeon/wgi/pipeline.h>
#include <pigeon/util.h>

struct VkShaderModule_T;
struct VkPipeline_T;
struct VkPipelineLayout_T;

typedef struct PigeonVulkanShader {
	struct VkShaderModule_T* vk_shader_module; // VkShaderModule
} PigeonVulkanShader;


ERROR_RETURN_TYPE pigeon_vulkan_load_shader(PigeonVulkanShader*, const char* file_path);
ERROR_RETURN_TYPE pigeon_vulkan_create_shader(PigeonVulkanShader*, const uint32_t* spv, unsigned int spv_size);
void pigeon_vulkan_destroy_shader(PigeonVulkanShader*);


typedef struct PigeonVulkanPipeline {
	struct VkPipeline_T* vk_pipeline;
	struct VkPipelineLayout_T* vk_pipeline_layout;
	unsigned char push_constants_size;
} PigeonVulkanPipeline;


// N.b. Shader objects can be deleted after creating a pipeline
int pigeon_vulkan_create_pipeline(PigeonVulkanPipeline*, PigeonVulkanShader* vs, PigeonVulkanShader* fs, 
	unsigned int push_constants_size, PigeonVulkanRenderPass*, PigeonVulkanDescriptorLayout*, 
	const PigeonWGIPipelineConfig*,

	// 1 value in sc_data for each specialisation constant. booleans are 0 (false) or 1 (true)
	unsigned int specialisation_constants, uint32_t * sc_data);

void pigeon_vulkan_destroy_pipeline(PigeonVulkanPipeline*);
