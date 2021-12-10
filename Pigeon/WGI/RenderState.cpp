#include "RenderState.h"
#include <cassert>
#include "WGI.h"
#include "Shader.h"
#include "../Vulkan/Pipeline.h"

using namespace std;

namespace WGI {

	// TODO constexpr
	uint32_t get_vertex_attribute_type_size(VertexAttributeType attribute_type)
	{
		switch (attribute_type) {
			case VertexAttributeType::Position:
			case VertexAttributeType::Colour:
				return 3 * 4;
			case VertexAttributeType::Position2D:
				return 2 * 4;
			case VertexAttributeType::PositionNormalised:
			case VertexAttributeType::Normal:
			case VertexAttributeType::Tangent:
			case VertexAttributeType::Texture:
			case VertexAttributeType::Bone:
			case VertexAttributeType::ColourRGBA8:
			case VertexAttributeType::ColourRGB8A2:
				return 4;
		}
		return 0;
	}



	PipelineInstance::PipelineInstance(WGI& wgi_, PipelineConfig cfg,
		Shader const& depth_vs, Shader const& vs, Shader const& fs)
	: wgi(wgi_)
	{
		assert_(&wgi == &depth_vs.get_wgi());
		assert_(&wgi == &vs.get_wgi());
		assert_(&wgi == &fs.get_wgi());

		PipelineConfig depth_cfg = cfg;

		// if(cfg.cull_mode == CullMode::Back) {
		// 	depth_cfg.cull_mode = CullMode::Front;
		// }
		// else if(cfg.cull_mode == CullMode::Front) {
		// 	depth_cfg.cull_mode = CullMode::Back;
		// }

		{ // Remove all but the positions attribute for depth-only render
			auto const& attributes = cfg.vertex_attributes;
			for(unsigned int i = 0; i < attributes.size(); i++) {
				if(attributes[i] == VertexAttributeType::Position2D or
					attributes[i] == VertexAttributeType::PositionNormalised or
					attributes[i] == VertexAttributeType::Position)
				{
					depth_cfg.vertex_attributes.clear();
					depth_cfg.vertex_attributes.push_back(attributes[i]);
					break;
				}
			}
		}

		depth_only_pipeline = Ref<Vulkan::Pipeline>::make(wgi.get_vulkan_device_(), *wgi.get_swapchain_(),
			depth_vs.get_shader_(), nullopt, depth_cfg, wgi.get_depth_renderpass_(), wgi.get_descriptor_layout_(), 
			4, true);

		shadow_pipeline = Ref<Vulkan::Pipeline>::make(wgi.get_vulkan_device_(), *wgi.get_swapchain_(),
			depth_vs.get_shader_(), nullopt, depth_cfg, wgi.get_shadow_renderpass_(), wgi.get_shadow_descriptor_layout_(), 
			4, true);

		cfg.depth_write = false;

		pipeline = Ref<Vulkan::Pipeline>::make(wgi.get_vulkan_device_(), *wgi.get_swapchain_(),
			vs.get_shader_(), fs.get_shader_(), cfg, wgi.get_main_renderpass_(), wgi.get_descriptor_layout_(), 
			4, false);
	}

	PipelineInstance::PipelineInstance(WGI& wgi_, PipelineConfig cfg,
		Shader const& vs, Shader const& fs)
	: wgi(wgi_)
	{
		assert_(&wgi == &vs.get_wgi());
		assert_(&wgi == &fs.get_wgi());

		cfg.depth_write = false;

		pipeline = Ref<Vulkan::Pipeline>::make(wgi.get_vulkan_device_(), *wgi.get_swapchain_(),
			vs.get_shader_(), fs.get_shader_(), cfg, wgi.get_main_renderpass_(), wgi.get_descriptor_layout_(), 
			4, false);
	}

	
	PipelineInstance::PipelineInstance(WGI& wgi_, PipelineConfig cfg,
		Shader const& vs, Shader const& fs, unsigned int push_constants_size,
		Vulkan::RenderPass const& rp, Vulkan::DescriptorLayout const& d_layout)
	: wgi(wgi_)
	{
		assert_(&wgi == &vs.get_wgi());
		assert_(&wgi == &fs.get_wgi());

		pipeline = Ref<Vulkan::Pipeline>::make(wgi.get_vulkan_device_(), *wgi.get_swapchain_(),
			vs.get_shader_(), fs.get_shader_(), cfg, rp, d_layout, 
			push_constants_size, false);
	}

	PipelineInstance::~PipelineInstance(){}
}