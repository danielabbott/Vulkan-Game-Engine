#pragma once

#include "ArrayPointer.h"
#include "HeapArray.h"
#include <nlohmann/json.hpp>
#include "IncludeGLM.h"
#include <vector>
#include "WGI/Mesh.h"

//struct Bone {
//	glm::vec3 position;
//	int parent = -1;
//};
//
//struct BoneExtra {
//	glm::vec3 end;
//	std::string name;
//};
//
//class Model {
//	HeapArray<uint8_t> data;
//	std::vector<Bone> bones;
//
//	MeshMeta mesh_meta;
//
//	// Points to data ^^
//	std::vector<ArrayPointer<uint8_t>> vertex_attribute_data;
//	ArrayPointer<uint8_t> index_data;
//
//
//public:
//
//	Model(HeapArray<uint8_t> && data, nlohmann::json const& json);
//
//	std::vector<Bone>&& take_bone_data() { return std::move(bones); }
//	std::vector<Bone> const& get_bone_data() const { return bones; }
//
//};


