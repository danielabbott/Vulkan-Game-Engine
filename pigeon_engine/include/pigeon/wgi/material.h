#pragma once

struct PigeonAsset;

typedef struct PigeonWGIMaterialImport {
	char* name;

	// Vertex/index
	unsigned int first;
	unsigned int count;

	float colour[3];
	float specular;

	char* texture;
	char* normal_map_texture;

	struct PigeonAsset* texture_asset;
	struct PigeonAsset* normal_map_texture_asset;
} PigeonWGIMaterialImport;
