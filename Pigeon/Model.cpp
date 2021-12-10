#include "Model.h"
//#include <unordered_map>
#include "deps/magic_enum.hpp"
#include "BetterAssert.h"
#include "WGI/RenderState.h"

using namespace std;

//Model::Model(HeapArray<uint8_t>&& data_, nlohmann::json const& json)
//	: data(move(data_))
//{
//
//	mesh_meta.vertex_count = json["vertex_count"];
//	mesh_meta.index_count = json["indices_count"];
//
//	auto const& indices_type_string = json["indices_type"];
//
//	uint32_t index_size_bytes = 2;
//	if (indices_type_string == "small") {
//		mesh_meta.big_indices = false;
//	}
//	else if (indices_type_string == "big") {
//		mesh_meta.big_indices = true;
//		index_size_bytes = 4;
//	}
//	else {
//		assert__(false, "Unrecognised index size");
//	}
//
//	assert__(mesh_meta.index_count * index_size_bytes == json["indices_data_size"], "Invalid indices_data_size");
//	index_data = data.slice(static_cast<uint32_t>(json["indices_data_offset"]), mesh_meta.index_count * index_size_bytes);
//
//	for (auto const& attrib_json : json["vertex_attributes"]) {
//		auto attrib = magic_enum::enum_cast<VertexAttributeType>(static_cast<string>(attrib_json["attribute_type"]));
//		assert__(attrib.has_value(), "Unrecognised vertex attribute");
//
//		uint32_t data_offset = attrib_json["data_offset"];
//		uint32_t attrib_size = get_vertex_attribute_type_size(attrib.value());
//
//		assert__(attrib_size * mesh_meta.vertex_count == static_cast<uint32_t>(attrib_json["data_size"]), "Invalid data_size");
//		vertex_attribute_data.push_back(data.slice(data_offset, attrib_size * mesh_meta.vertex_count));
//		
//		mesh_meta.vertex_attributes.push_back(attrib.value());
//	}
//
//}

//Mesh Model::create_mesh() const
//{
//	return Mesh(mesh_meta, move(data), uint32_t index_data_offset, ArrayPointer<const uint32_t> vertex_data_offsets);
//}