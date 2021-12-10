#pragma once

#include "RenderState.h"
#include <vector>
#include "../Ref.h"
#include "../ArrayPointer.h"
#include "../Model.h"
#include <memory>
#include "../IncludeGLM.h"
#include <optional>

namespace Vulkan {
	class AutoBuffer;
	class Buffer;
	class Device;
	class CommandPool;
}
class EngineState;

namespace WGI {

	class WGI;
	class CmdBuffer;


	struct MeshMeta {
		std::vector<VertexAttributeType> vertex_attributes;
		std::vector<uint64_t> vertex_offsets; // offset into buffer in bytes;
		uint64_t indices_offset = UINT64_MAX;
		unsigned int vertex_count = 0;
		unsigned int index_count = 0;
		bool big_indices = false;

		struct Bounds {
			glm::vec3 minimum;
			glm::vec3 range;
		};
		std::optional<Bounds> bounds;

		glm::mat4 get_matrix() const {
			if(!bounds.has_value()) {
				return glm::mat4(1.0f);
			}
			return glm::translate(glm::mat4(1.0f), bounds.value().minimum)*glm::scale(bounds.value().range);
		}

		bool has_position_bounds() const { return bounds.has_value(); }
		glm::vec3 get_position_bounds_minimum() const { return bounds.value().minimum; }
		glm::vec3 get_position_bounds_range() const { return bounds.value().range; }
	};

	// Mesh on GPU (data is not stored client-side)
	class Mesh {
		WGI& wgi;
		MeshMeta meta;
		std::unique_ptr<Vulkan::AutoBuffer> buffer;

		uint32_t data_size = 0;

	public:
		Mesh(WGI&, MeshMeta meta);
		~Mesh();

		MeshMeta const& get_meta() const { return meta; }

		ArrayPointer<uint8_t> create_staging_buffer(uint32_t size);

		void upload_(Vulkan::CommandPool &);

		 // TODO
		// void upload_async();
		// bool is_upload_complete() const;

		Vulkan::Buffer const& get_buffer_() const;

	};

}