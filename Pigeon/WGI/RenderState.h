#pragma once

#include <cstdint>
#include <vector>
#include "../Ref.h"
#include "../BetterAssert.h"
#include "WGI.h"

namespace Vulkan {
	class Pipeline;
	class Device;
}

namespace WGI {

	class Shader;
	class WGI;

	enum class CullMode {
		None,
		Front,
		Back
	};

	enum class FrontFace {
		Clockwise,
		Anticlockwise,
	};

	enum class BlendFunction {
		None,
		Normal
		// TODO blend modes needed for subpixel font rendering
	};

	enum class VertexAttributeType {
		Position, // VK_FORMAT_R32G32B32_SFLOAT
		Position2D, // VK_FORMAT_R32G32_SFLOAT

		// LSB of A is bit 11 of R (X)
		// MSB of A is bit 11 of G (Y)
		PositionNormalised, // VK_FORMAT_A2B10G10R10_UINT_PACK32

		Colour, // VK_FORMAT_R32G32B32_SFLOAT
		ColourRGBA8, // VK_FORMAT_A8B8G8R8_UNORM_PACK32
		ColourRGB8A2, // VK_FORMAT_A2B10G10R10_UNORM_PACK32

		Normal, // VK_FORMAT_A2B10G10R10_UNORM_PACK32

		Tangent, // VK_FORMAT_A2B10G10R10_UNORM_PACK32

		Texture, // VK_FORMAT_R16G16_UNORM
		Bone // VK_FORMAT_R8G8B8A8_UINT - 2 bone indices and 2 bone weights
	};

	uint32_t get_vertex_attribute_type_size(VertexAttributeType);


	struct PipelineConfig {
		// ** Remember to add new variables to the == function **

		CullMode cull_mode = CullMode::Back;
		FrontFace front_face = FrontFace::Anticlockwise;
		BlendFunction blend_function = BlendFunction::None;
		bool depth_test = true;
		bool depth_write = true;

		std::vector<VertexAttributeType> vertex_attributes;

		PipelineConfig() {}

		PipelineConfig(std::vector<VertexAttributeType> && vertex_attributes_)
		: vertex_attributes(std::move(vertex_attributes_))
		{}

		bool operator==(const PipelineConfig& b)
		{
			return
				cull_mode == b.cull_mode and
				front_face == b.front_face and
				blend_function == b.blend_function and
				vertex_attributes == b.vertex_attributes
			;
		}

		bool operator!=(const PipelineConfig& rhs){ return !(*this == rhs); }

		// C++20: bool operator==(const Point&) const = default;
	};

	enum class PipelineType {
		Normal, DepthOnly, Shadow
	};

	class PipelineInstance {
		WGI& wgi;
		Ref<Vulkan::Pipeline> pipeline;
		Ref<Vulkan::Pipeline> depth_only_pipeline;
		Ref<Vulkan::Pipeline> shadow_pipeline;

	public:

		Ref<Vulkan::Pipeline> const& get_pipeline_(PipelineType t) const {
			switch(t) {
				case PipelineType::Normal:
					return pipeline;
				case PipelineType::DepthOnly:
					return depth_only_pipeline;
				case PipelineType::Shadow:
					return shadow_pipeline;				
			}
			return pipeline;
		}

		PipelineInstance(WGI&, PipelineConfig cfg,
			Shader const& depth_vs, Shader const& vs, Shader const& fs);
		PipelineInstance(WGI&, PipelineConfig cfg,
			Shader const& vs, Shader const& fs);
		~PipelineInstance();


		// Used within WGI module only.
		PipelineInstance(WGI&, PipelineConfig cfg, 
			Shader const& vs, Shader const& fs, 
			unsigned int push_constants_size, Vulkan::RenderPass const&, Vulkan::DescriptorLayout const&);
	};
}