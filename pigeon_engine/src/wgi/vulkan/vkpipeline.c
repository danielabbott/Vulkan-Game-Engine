#include "singleton.h"
#include <pigeon/assert.h>
#include <pigeon/misc.h>
#include <pigeon/wgi/vulkan/pipeline.h>

PIGEON_ERR_RET pigeon_vulkan_create_shader(PigeonVulkanShader* shader, const uint32_t* spv, unsigned int spv_size)
{
	assert(shader && spv && spv_size);

	VkShaderModuleCreateInfo create_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	create_info.codeSize = spv_size;
	create_info.pCode = spv;

	int err = vkCreateShaderModule(vkdev, &create_info, NULL, &shader->vk_shader_module);

	ASSERT_LOG_R1(err == VK_SUCCESS, "vkCreateShaderModule error");
	return 0;
}

static VkFormat get_vertex_attribute_format(PigeonWGIVertexAttributeType type)
{
	switch (type) {
	case PIGEON_WGI_VERTEX_ATTRIBUTE_NONE:
		return 0;

	case PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION:
	case PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR:
		return VK_FORMAT_R32G32B32_SFLOAT;

	case PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION2D:
	case PIGEON_WGI_VERTEX_ATTRIBUTE_UV_FLOAT:
		return VK_FORMAT_R32G32_SFLOAT;
	case PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED:
		return VK_FORMAT_A2B10G10R10_UINT_PACK32;

	case PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR_RGBA8:
		return VK_FORMAT_A8B8G8R8_UNORM_PACK32;

	case PIGEON_WGI_VERTEX_ATTRIBUTE_NORMAL:
	case PIGEON_WGI_VERTEX_ATTRIBUTE_TANGENT:
		return VK_FORMAT_A2B10G10R10_SNORM_PACK32;

	case PIGEON_WGI_VERTEX_ATTRIBUTE_UV:
		return VK_FORMAT_R16G16_UNORM;
	case PIGEON_WGI_VERTEX_ATTRIBUTE_BONE:
		return VK_FORMAT_R16G16_UINT;
	}
	return 0;
}

PIGEON_ERR_RET pigeon_vulkan_load_shader(PigeonVulkanShader* shader, const char* file_path)
{
	assert(shader && file_path);

	unsigned long file_size;
	uint32_t* binary_data = (uint32_t*)pigeon_load_file(file_path, 0, &file_size);
	ASSERT_R1(binary_data);

	int err = pigeon_vulkan_create_shader(shader, binary_data, (uint32_t)file_size);

	free(binary_data);

	ASSERT_R1(!err);
	return 0;
}

void pigeon_vulkan_destroy_shader(PigeonVulkanShader* shader)
{
	assert(shader);
	assert(shader->vk_shader_module);
	if (shader && shader->vk_shader_module) {
		vkDestroyShaderModule(vkdev, (VkShaderModule)shader->vk_shader_module, NULL);
		shader->vk_shader_module = NULL;
	}
}

static PIGEON_ERR_RET create_layout(
	VkPipelineLayout* layout, PigeonVulkanDescriptorLayout* descriptor_layout, unsigned int push_constants_size)
{
	ASSERT_R1(layout && !*layout);
	if (descriptor_layout)
		ASSERT_R1(descriptor_layout->handle);

	VkPipelineLayoutCreateInfo pipeline_layout_create = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	VkDescriptorSetLayout desc = VK_NULL_HANDLE;
	if (descriptor_layout) {
		pipeline_layout_create.setLayoutCount = 1;
		desc = (VkDescriptorSetLayout)descriptor_layout->handle;
		pipeline_layout_create.pSetLayouts = &desc;
	}

	VkPushConstantRange push_constants = { 0 };

	if (push_constants_size) {
		push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		push_constants.size = push_constants_size;
		pipeline_layout_create.pushConstantRangeCount = 1;
		pipeline_layout_create.pPushConstantRanges = &push_constants;
	}

	ASSERT_LOG_R1(vkCreatePipelineLayout(vkdev, &pipeline_layout_create, NULL, layout) == VK_SUCCESS,
		"vkCreatePipelineLayout error");
	return 0;
}

// N.b. Shader objects can be deleted after creating a pipeline
PIGEON_ERR_RET pigeon_vulkan_create_pipeline(PigeonVulkanPipeline* pipeline, PigeonVulkanShader* vs,
	PigeonVulkanShader* fs, unsigned int push_constants_size, PigeonVulkanRenderPass* render_pass,
	PigeonVulkanDescriptorLayout* descriptor_layout, const PigeonWGIPipelineConfig* cfg,
	unsigned int specialisation_constants, uint32_t* sc_data)
{
	if (!pipeline || !vs || !render_pass || !render_pass->vk_renderpass || push_constants_size > 128
		|| specialisation_constants > 16) {
		ASSERT_R1(false);
	}

	ASSERT_R1(!create_layout(&pipeline->vk_pipeline_layout, descriptor_layout, push_constants_size));

	VkPipelineShaderStageCreateInfo shaders[2] = { { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
		{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO } };

	VkSpecializationInfo sc_info = { 0 };
	VkSpecializationMapEntry sc_maps[16];

	sc_info.mapEntryCount = specialisation_constants;
	sc_info.pMapEntries = sc_maps;
	sc_info.dataSize = specialisation_constants * 4;
	sc_info.pData = sc_data;

	for (unsigned int i = 0; i < specialisation_constants; i++) {
		sc_maps[i].constantID = i;
		sc_maps[i].offset = i * 4;
		sc_maps[i].size = 4;
	}

	shaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaders[0].module = vs->vk_shader_module;
	shaders[0].pName = "main";
	shaders[0].pSpecializationInfo = &sc_info;

	if (fs) {
		shaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaders[1].module = fs->vk_shader_module;
		shaders[1].pName = "main";
		shaders[1].pSpecializationInfo = &sc_info;
	}

	VkPipelineVertexInputStateCreateInfo vertex_data_format
		= { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	VkVertexInputAttributeDescription vertex_attributes[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES] = { { 0 } };
	VkVertexInputBindingDescription bindings[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES] = { { 0 } };

	unsigned int vertex_attrib_count = 0;
	for (; vertex_attrib_count < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES; vertex_attrib_count++) {
		PigeonWGIVertexAttributeType type = cfg->vertex_attributes[vertex_attrib_count];
		if (!type)
			break;
		VkVertexInputAttributeDescription* attr = &vertex_attributes[vertex_attrib_count];
		VkVertexInputBindingDescription* bind = &bindings[vertex_attrib_count];

		bind->binding = vertex_attrib_count;
		bind->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		bind->stride = pigeon_wgi_get_vertex_attribute_type_size(type);

		attr->binding = vertex_attrib_count;
		attr->location = vertex_attrib_count;
		attr->format = get_vertex_attribute_format(type);
	}

	vertex_data_format.pVertexAttributeDescriptions = vertex_attributes;
	vertex_data_format.pVertexBindingDescriptions = bindings;
	vertex_data_format.vertexAttributeDescriptionCount = vertex_attrib_count;
	vertex_data_format.vertexBindingDescriptionCount = vertex_attrib_count;

	VkPipelineInputAssemblyStateCreateInfo topology = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	topology.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewport_state = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;

	VkPipelineDynamicStateCreateInfo dynamic_state = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	VkDynamicState viewport_dynamic_state[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	dynamic_state.dynamicStateCount = 2;
	dynamic_state.pDynamicStates = viewport_dynamic_state;

	VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizer.depthClampEnable = singleton_data.depth_clamp_supported;
	rasterizer.lineWidth = 1.0f;

	rasterizer.frontFace = cfg->front_face == PIGEON_WGI_FRONT_FACE_CLOCKWISE ? VK_FRONT_FACE_CLOCKWISE
																			  : VK_FRONT_FACE_COUNTER_CLOCKWISE;

	if (cfg->cull_mode == PIGEON_WGI_CULL_MODE_FRONT) {
		rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	} else if (cfg->cull_mode == PIGEON_WGI_CULL_MODE_BACK) {
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	} else {
		rasterizer.cullMode = VK_CULL_MODE_NONE;
	}
	rasterizer.polygonMode = cfg->wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;

	if (cfg->depth_bias) {
		rasterizer.depthBiasEnable = true;
		rasterizer.depthBiasSlopeFactor = -1;
	}

	VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depth_state = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	if (cfg->depth_test) {
		depth_state.depthTestEnable = VK_TRUE;
	}
	if (cfg->depth_write) {
		depth_state.depthWriteEnable = VK_TRUE;
	}
	if (cfg->depth_cmp_equal) {
		depth_state.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
	} else {
		depth_state.depthCompareOp = VK_COMPARE_OP_GREATER;
	}

	VkPipelineColorBlendAttachmentState blend[2] = { { 0 } };
	blend[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
	blend[1].colorWriteMask
		= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	if (!cfg->depth_only && cfg->blend_function == PIGEON_WGI_BLEND_NORMAL) {
		blend[0].blendEnable = VK_TRUE;
		blend[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blend[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blend[0].colorBlendOp = VK_BLEND_OP_ADD;
		blend[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blend[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blend[0].alphaBlendOp = VK_BLEND_OP_ADD;
	} else {
		blend[0].colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo blend2 = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	blend2.attachmentCount = render_pass->has_2_colour_images ? 2 : 1;
	blend2.pAttachments = blend;

	VkGraphicsPipelineCreateInfo pipeline_create = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipeline_create.stageCount = fs ? 2 : 1;
	pipeline_create.pStages = shaders;
	pipeline_create.pVertexInputState = &vertex_data_format;
	pipeline_create.pInputAssemblyState = &topology;
	pipeline_create.pViewportState = &viewport_state;
	pipeline_create.pDynamicState = &dynamic_state;
	pipeline_create.pRasterizationState = &rasterizer;
	pipeline_create.pMultisampleState = &multisampling;
	pipeline_create.pDepthStencilState = &depth_state;
	pipeline_create.pColorBlendState = cfg->depth_only ? VK_NULL_HANDLE : &blend2;
	pipeline_create.layout = pipeline->vk_pipeline_layout;
	pipeline_create.renderPass = render_pass->vk_renderpass;
	pipeline_create.subpass = 0;
	pipeline_create.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(vkdev, VK_NULL_HANDLE, 1, &pipeline_create, NULL, &pipeline->vk_pipeline)
		!= VK_SUCCESS) {
		vkDestroyPipelineLayout(vkdev, pipeline->vk_pipeline_layout, NULL);
		ASSERT_LOG_R1(false, "vkCreateGraphicsPipelines error");
	}
	return 0;
}

void pigeon_vulkan_destroy_pipeline(PigeonVulkanPipeline* pipeline)
{
	assert(pipeline);
	if (pipeline) {
		if (pipeline->vk_pipeline) {
			vkDestroyPipeline(vkdev, pipeline->vk_pipeline, NULL);
			pipeline->vk_pipeline = NULL;
		}
		if (pipeline->vk_pipeline_layout) {
			vkDestroyPipelineLayout(vkdev, pipeline->vk_pipeline_layout, NULL);
			pipeline->vk_pipeline_layout = NULL;
		}
	}
}
