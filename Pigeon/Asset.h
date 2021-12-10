#pragma once

#include <string>
#include "HeapArray.h"
#include <variant>
#include "Model.h"
#include <string_view>
#include <nlohmann/json.hpp>
#include <cstdint>

enum class AssetState {
	NotLoaded, DataLoaded, Decoded
};

class Asset {
	//AssetState state;

	std::string json_file_path;
	nlohmann::json metadata;

	HeapArray<uint8_t> data_file_contents;
	//ArrayPointer<uint8_t> data_file_contents2;

	HeapArray<uint8_t> data_file_compressed_contents;

	uint32_t decoded_size = 0;

	// std::monostate means data is empty - text or binary_blob type, use data_file_contents
	//std::variant<std::monostate, Model> data;
public:
	Asset(std::string json_file_path);
	void load_from_disk();

	// Decompresses data. data_file_contents is set if data is std::monostate, otherwise data variable takes the HeapArray
	void decompress();
	void decompress(ArrayPointer<uint8_t> memory_to_use);

	uint32_t get_decompressed_size() const;


	std::basic_string_view<uint8_t> get_data() const;
	uint32_t data_size() const;

	//Model get_model() const;
	//AssetState get_state() const { return state; }

	WGI::MeshMeta get_mesh_meta() const;
};
