#include "Asset.h"
#include "FileIO.h"
#include <zstd.h>
#include "Model.h"
#include "deps/magic_enum.hpp"
#include <spdlog/spdlog.h>

using namespace std;
using namespace nlohmann;

Asset::Asset(std::string json_file_path_)
	:json_file_path(json_file_path_)
{
	auto json_file_contents = load_file_blocking(json_file_path, 1);
	json_file_contents.get()[json_file_contents.size() - 1] = 0;
	metadata = json::parse(reinterpret_cast<char*>(json_file_contents.get()));
}

uint32_t Asset::get_decompressed_size() const {
	return metadata["decompressed_size"];
}

void Asset::load_from_disk()
{
	string data_file_path = json_file_path.substr(0, json_file_path.size() - 4).append("pigeon-data");

	data_file_compressed_contents = load_file_blocking(data_file_path);
}
void Asset::decompress()
{	
	assert_(data_file_compressed_contents.size());

	auto compression_type = metadata["compression"];
	if (compression_type == "none") {
		data_file_contents = move(data_file_compressed_contents);
	}
	else {
		data_file_contents = HeapArray<uint8_t>(metadata["decompressed_size"]);
		decompress(data_file_contents.get_array_ptr());
	}
}

void Asset::decompress(ArrayPointer<uint8_t> memory_to_use)
{
	assert_(data_file_compressed_contents.size());

	auto compression_type = metadata["compression"];
	if (compression_type == "zstd") {
		uint32_t expected_size = metadata["decompressed_size"];
		assert_(memory_to_use.size() >= expected_size);
		auto output_bytes_count = ZSTD_decompress(memory_to_use.get(), expected_size, data_file_compressed_contents.get(), data_file_compressed_contents.size());

		if(ZSTD_isError(output_bytes_count)) {
			spdlog::error("ZSTD error: {}", ZSTD_getErrorName(output_bytes_count));
			assert__(0, "Asset decompression error");
		}

		assert__(output_bytes_count == expected_size, "Asset decompression error (2)");
		data_file_compressed_contents.~HeapArray();
	}
	else {
		assert__(compression_type == "none", "Unrecognised asset compression type");
		assert_(memory_to_use.size() >= data_file_compressed_contents.size());
		memcpy(memory_to_use.get(), data_file_compressed_contents.get(), data_file_compressed_contents.size());
	}

}

//Model Asset::get_model() const
//{
//	assert__(metadata["asset_type"] == "model", "Unrecognised asset type");
//	return Model(move(data_file_contents), metadata);
//}

basic_string_view<uint8_t> Asset::get_data() const
{
	//assert__(holds_alternative<monostate>(data), "Wrong asset type");
	assert__(data_file_contents.size(), "No data");
	return basic_string_view<uint8_t>(data_file_contents.get(), data_file_contents.count());
}

//Model const& Asset::get_model() const
//{
//	return get<Model>(data);
//}


WGI::MeshMeta Asset::get_mesh_meta() const
{
	assert__(metadata["asset_type"] == "model", "Unrecognised asset type");

	WGI::MeshMeta mesh_meta;
	mesh_meta.vertex_count = metadata["vertex_count"];
	mesh_meta.index_count = metadata["indices_count"];
	mesh_meta.indices_offset = metadata["indices_data_offset"];

	bool has_bounds = metadata.contains("bounds_minimum") && metadata.contains("bounds_range");

	if(has_bounds) {
		auto min = metadata["bounds_minimum"];
		auto range = metadata["bounds_range"];
		mesh_meta.bounds = WGI::MeshMeta::Bounds {
			glm::vec3(min[0], min[1], min[2]),
			glm::vec3(range[0], range[1], range[2])
		};
	}

	auto const& indices_type_string = metadata["indices_type"];

	uint32_t index_size_bytes = 2;
	if (indices_type_string == "small") {
		mesh_meta.big_indices = false;
	}
	else if (indices_type_string == "big") {
		mesh_meta.big_indices = true;
		index_size_bytes = 4;
	}
	else {
		assert__(false, "Unrecognised index size");
	}

	assert__(mesh_meta.index_count * index_size_bytes == metadata["indices_data_size"], "Invalid indices_data_size");
	//index_data = data.slice(static_cast<uint32_t>(metadata["indices_data_offset"]), mesh_meta.index_count * index_size_bytes);

	for (auto const& attrib_json : metadata["vertex_attributes"]) {
		auto attrib = magic_enum::enum_cast<WGI::VertexAttributeType>(static_cast<string>(attrib_json["attribute_type"]));
		assert__(attrib.has_value(), "Unrecognised vertex attribute");

		if(attrib == WGI::VertexAttributeType::PositionNormalised) {
			assert__(has_bounds, "PositionNormalised requires bounds information");
		}

		// uint32_t data_offset = attrib_json["data_offset"];
		uint32_t attrib_size = get_vertex_attribute_type_size(attrib.value());

		assert__(attrib_size * mesh_meta.vertex_count == static_cast<uint32_t>(attrib_json["data_size"]), "Invalid data_size");
		//vertex_attribute_data.push_back(data.slice(data_offset, attrib_size * mesh_meta.vertex_count));

		mesh_meta.vertex_attributes.push_back(attrib.value());
		mesh_meta.vertex_offsets.push_back(attrib_json["data_offset"]);
	}

	return mesh_meta;
}
