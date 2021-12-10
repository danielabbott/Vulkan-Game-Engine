#include "Pipeline.h"
#include "../IncludeSpdlog.h"
#include "Device.h"
#include "Include.h"
#include "DescriptorSets.h"
#include <array>

using namespace std;

namespace Vulkan {

	Pipeline::Pipeline(Device& device_, Swapchain const& swapchain,
		Shader const& vs, optional<reference_wrapper<const Shader>> fs, WGI::PipelineConfig cfg,
		RenderPass const& render_pass, optional<reference_wrapper<const DescriptorLayout>>  descriptor_layout,
		unsigned int push_constants_size_, bool depth_only)
		: device(device_), push_constants_size(push_constants_size_)
	{
		assert_(push_constants_size <= 128);

		/* Shader stages */
		vector<VkPipelineShaderStageCreateInfo> shaders;
		shaders.resize(1);

		shaders[0] = {};
		shaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaders[0].module = vs.get_handle();
		shaders[0].pName = "main";

		if(fs.has_value()) {
			shaders.resize(2);
			shaders[1] = {};
			shaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaders[1].module = fs.value().get().get_handle();
			shaders[1].pName = "main";
		}



		VkPipelineVertexInputStateCreateInfo vertex_data_format{};
		vertex_data_format.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		vector<VkVertexInputBindingDescription> bindings;
		vector<VkVertexInputAttributeDescription> attributes;

		for (unsigned int i = 0; i < cfg.vertex_attributes.size(); i++) {
			auto attribute_type = cfg.vertex_attributes[i];
			VkVertexInputBindingDescription binding{};

			binding.binding = i;
			binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			binding.stride = get_vertex_attribute_type_size(attribute_type);
			assert__(binding.stride, "Unknown attribute type");


			bindings.push_back(binding);

			VkVertexInputAttributeDescription attribute;
			attribute.binding = i;
			attribute.location = i;
			attribute.offset = 0;

			switch (attribute_type) {
			case WGI::VertexAttributeType::Position:
			case WGI::VertexAttributeType::Colour:
				attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
				break;
			case WGI::VertexAttributeType::Position2D:
				attribute.format = VK_FORMAT_R32G32_SFLOAT;
				break;
			case WGI::VertexAttributeType::PositionNormalised:
				attribute.format = VK_FORMAT_A2B10G10R10_UINT_PACK32;
				break;
			case WGI::VertexAttributeType::Normal:
			case WGI::VertexAttributeType::ColourRGB8A2:
			case WGI::VertexAttributeType::Tangent:
				attribute.format = VK_FORMAT_A2B10G10R10_SNORM_PACK32;
				break;
			case WGI::VertexAttributeType::Texture:
				attribute.format = VK_FORMAT_R16G16_UNORM;
				break;
			case WGI::VertexAttributeType::Bone:
				attribute.format = VK_FORMAT_R8G8B8A8_UINT;
				break;
			case WGI::VertexAttributeType::ColourRGBA8:
				attribute.format = VK_FORMAT_A8B8G8R8_UNORM_PACK32;
				break;
			default:
				assert__(false, "Unknown attribute type");
			}

			attributes.push_back(attribute);
		}

		if (bindings.size() > 0) {
			vertex_data_format.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
			vertex_data_format.pVertexBindingDescriptions = bindings.data();
			vertex_data_format.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
			vertex_data_format.pVertexAttributeDescriptions = attributes.data();
		}

		VkPipelineInputAssemblyStateCreateInfo topology{};
		topology.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		topology.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;



		VkPipelineDynamicStateCreateInfo dynamic_state { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		array<VkDynamicState, 2> viewport_dynamic_state = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		dynamic_state.dynamicStateCount = viewport_dynamic_state.size();
		dynamic_state.pDynamicStates = viewport_dynamic_state.data();

		VkViewport viewport{};
		viewport.width = static_cast<float>(swapchain.get_width());
		viewport.height = static_cast<float>(swapchain.get_height());
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.extent = VkExtent2D{ swapchain.get_width(), swapchain.get_height() };


		VkPipelineViewportStateCreateInfo viewport_state{};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports = &viewport;
		viewport_state.scissorCount = 1;
		viewport_state.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = device.supports_depth_clamp();
		rasterizer.lineWidth = 1.0f;

		rasterizer.frontFace = cfg.front_face == WGI::FrontFace::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;

		if (cfg.cull_mode == WGI::CullMode::Front) {
			rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
		}
		else if (cfg.cull_mode == WGI::CullMode::Back) {
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		}
		else {
			rasterizer.cullMode = VK_CULL_MODE_NONE;
		}

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depth_state{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
		if(cfg.depth_test) {
			depth_state.depthTestEnable = VK_TRUE;
		}
		if(cfg.depth_write) {
			depth_state.depthWriteEnable = VK_TRUE;
		}
		if(depth_only) {
			depth_state.depthCompareOp = VK_COMPARE_OP_LESS;
		}
		else {
			depth_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		}


		VkPipelineColorBlendAttachmentState blend{};
		blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		if (cfg.blend_function == WGI::BlendFunction::Normal) {
			blend.blendEnable = VK_TRUE;
			blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blend.colorBlendOp = VK_BLEND_OP_ADD;
			blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			blend.alphaBlendOp = VK_BLEND_OP_ADD;
		}

		VkPipelineColorBlendStateCreateInfo blend2{};
		blend2.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blend2.attachmentCount = 1;
		blend2.pAttachments = &blend;


		VkPipelineLayoutCreateInfo pipeline_layout_create{};
		pipeline_layout_create.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		VkDescriptorSetLayout desc;
		if (descriptor_layout.has_value()) {
			pipeline_layout_create.setLayoutCount = 1;
			desc = descriptor_layout.value().get().get_handle_();
			pipeline_layout_create.pSetLayouts = &desc;
		}

		VkPushConstantRange push_constants {};
		// push_constants.stageFlags = VK_SHADER_STAGE_ALL;
		push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
		push_constants.size = push_constants_size;

		if(push_constants_size) {
			pipeline_layout_create.pushConstantRangeCount = 1;
			pipeline_layout_create.pPushConstantRanges = &push_constants;
		}

		assert__(vkCreatePipelineLayout(
			device.get_device_handle_(), &pipeline_layout_create, nullptr, &pipeline_layout) == VK_SUCCESS,
			"vkCreatePipelineLayout error");




		VkGraphicsPipelineCreateInfo pipeline_create{};
		pipeline_create.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create.stageCount = shaders.size();
		pipeline_create.pStages = shaders.data();
		pipeline_create.pVertexInputState = &vertex_data_format;
		pipeline_create.pInputAssemblyState = &topology;
		pipeline_create.pViewportState = &viewport_state; // TODO do we need this? (if not get rid of swapchain argument)
		pipeline_create.pDynamicState = &dynamic_state;
		pipeline_create.pRasterizationState = &rasterizer;
		pipeline_create.pMultisampleState = &multisampling;
		pipeline_create.pDepthStencilState = &depth_state;
		pipeline_create.pColorBlendState = depth_only ? VK_NULL_HANDLE : &blend2;
		pipeline_create.layout = pipeline_layout;
		pipeline_create.renderPass = render_pass.get_handle();
		pipeline_create.subpass = 0;
		pipeline_create.basePipelineIndex = -1;


		assert__(vkCreateGraphicsPipelines(device.get_device_handle_(),
			VK_NULL_HANDLE, 1, &pipeline_create, nullptr, &graphics_pipeline) == VK_SUCCESS, "vkCreateGraphicsPipelines error");




	}

	Pipeline::~Pipeline()
	{
		auto d = device.get_device_handle_();
		vkDestroyPipeline(d, graphics_pipeline, nullptr);
		vkDestroyPipelineLayout(d, pipeline_layout, nullptr);
	}


}