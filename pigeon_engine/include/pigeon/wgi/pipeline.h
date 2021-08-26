#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <pigeon/util.h>

#define PIGEON_WGI_MAX_VERTEX_ATTRIBUTES 8

struct PigeonVulkanPipeline;
struct PigeonVulkanShader;

typedef enum {
	PIGEON_WGI_CULL_MODE_NONE,
	PIGEON_WGI_CULL_MODE_FRONT,
	PIGEON_WGI_CULL_MODE_BACK
} PigeonWGICullMode;

typedef enum {
	PIGEON_WGI_FRONT_FACE_ANTICLOCKWISE,
	PIGEON_WGI_FRONT_FACE_CLOCKWISE
} PigeonWGIFrontFace;

typedef enum {
	PIGEON_WGI_BLEND_NONE,
	PIGEON_WGI_BLEND_NORMAL
	// TODO blend modes needed for subpixel font rendering
} PigeonWGIBlendFunction;

typedef enum {
	PIGEON_WGI_VERTEX_ATTRIBUTE_NONE,
	PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION, // VK_FORMAT_R32G32B32_SFLOAT
	PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION2D, // VK_FORMAT_R32G32_SFLOAT

	// LSB of A is bit 0 of R (X)
	// MSB of A is bit 0 of G (Y)
	PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED, // VK_FORMAT_A2B10G10R10_UINT_PACK32

	PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR, // VK_FORMAT_R32G32B32_SFLOAT
	PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR_RGBA8, // VK_FORMAT_A8B8G8R8_UNORM_PACK32

	PIGEON_WGI_VERTEX_ATTRIBUTE_NORMAL, // VK_FORMAT_A2B10G10R10_UNORM_PACK32

	PIGEON_WGI_VERTEX_ATTRIBUTE_TANGENT, // VK_FORMAT_A2B10G10R10_UNORM_PACK32

	PIGEON_WGI_VERTEX_ATTRIBUTE_UV, // VK_FORMAT_R16G16_UNORM
	PIGEON_WGI_VERTEX_ATTRIBUTE_BONE2 // VK_FORMAT_R8G8B8A8_UINT - 2 bone indices and 2 bone weights
} PigeonWGIVertexAttributeType;

uint32_t pigeon_wgi_get_vertex_attribute_type_size(PigeonWGIVertexAttributeType);

typedef struct PigeonWGIPipelineConfig {
	PigeonWGICullMode cull_mode;
	PigeonWGIFrontFace front_face;
	PigeonWGIBlendFunction blend_function;
	bool depth_test;
	bool depth_write;
	bool depth_only;

	PigeonWGIVertexAttributeType vertex_attributes[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES];
} PigeonWGIPipelineConfig;

typedef enum {
	PIGEON_WGI_SHADER_TYPE_VERTEX,
	PIGEON_WGI_SHADER_TYPE_FRAGMENT
} PigeonWGIShaderType;

typedef struct PigeonWGIShader {
	struct PigeonVulkanShader * shader;
} PigeonWGIShader;

ERROR_RETURN_TYPE pigeon_wgi_create_shader(PigeonWGIShader*, const void* spv, uint32_t spv_size, PigeonWGIShaderType type);
void pigeon_wgi_destroy_shader(PigeonWGIShader*);

typedef struct PigeonWGIPipeline {
	struct PigeonVulkanPipeline * pipeline;
	struct PigeonVulkanPipeline * pipeline_depth_only;
	bool has_fragment_shader;
} PigeonWGIPipeline;

// Shaders can be destroyed after creating the pipeline.
int pigeon_wgi_create_pipeline(PigeonWGIPipeline*, PigeonWGIShader* vs, PigeonWGIShader* vs_depth_only, 
	PigeonWGIShader* fs, const PigeonWGIPipelineConfig*);
void pigeon_wgi_destroy_pipeline(PigeonWGIPipeline*);
