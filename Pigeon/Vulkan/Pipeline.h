#pragma once

#include "Shader.h"
#include "Swapchain.h"
#include "RenderPass.h"
#include "../WGI/RenderState.h"

#include "../Ref.h"

struct VkPipelineLayout_T;
struct VkPipeline_T;
struct VkDescriptorSetLayout_T;

namespace Vulkan {

	class DescriptorLayout;

	class Pipeline {
		Device& device;
		VkPipelineLayout_T* pipeline_layout = nullptr;
		VkPipeline_T* graphics_pipeline = nullptr;
		unsigned int push_constants_size;
	public:
		// Pipeline must only be used with render passes that are compatible with the one given to this constructor
		Pipeline(Device&, Swapchain const& swapchain,
			Shader const& vs, std::optional<std::reference_wrapper<const Shader>> fs, WGI::PipelineConfig,
			RenderPass const&, std::optional<std::reference_wrapper<const DescriptorLayout>> descriptor_layout,
			unsigned int push_constants_size, bool depth_enabled);

		VkPipeline_T* get_handle() const { return graphics_pipeline; }
		VkPipelineLayout_T* get_layout() const { return pipeline_layout; }
		unsigned int get_push_constants_size() const { return push_constants_size; }

		~Pipeline();
	};

}