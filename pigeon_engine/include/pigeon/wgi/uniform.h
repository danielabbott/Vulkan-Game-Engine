#pragma once

#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <cglm/types.h>
#include <stdint.h>

#define PIGEON_WGI_ALPHA_CHANNEL_UNUSED 0.0f
#define PIGEON_WGI_ALPHA_CHANNEL_UNDER_COLOUR 1.0f
#define PIGEON_WGI_ALPHA_CHANNEL_TRANSPARENCY 2.0f

typedef struct PigeonWGIDrawObject {
	mat4 proj_view_model[5];

	mat4 model;
	mat4 normal_model_matrix;

	vec3 position_min;
	float first_bone_index;
	vec3 position_range;
	float ssao_intensity;

	uint32_t texture_sampler_index_plus1; // into array of glsl samplers
	float texture_index; // into array texture

	int rsvd0;

	float specular_intensity;

	vec3 colour;
	float luminosity;

	// Textures with alpha channel interpolate between this colour and texture
	vec3 under_colour;
	float alpha_channel_usage;

	uint32_t normal_map_sampler_index_plus1;
	float normal_map_index;

	int rsvd1;
	int rsvd2;
} PigeonWGIDrawObject;

typedef struct PigeonWGILight {
	vec3 world_position;
	float light_type; // 0: directional, 1: point
	vec3 neg_direction;
	float is_shadow_caster; // 0 -> no shadows
	vec3 intensity;
	float shadow_pixel_offset;
	mat4 shadow_proj_view;
} PigeonWGILight;

typedef struct PigeonWGISceneUniformData {
	mat4 view;
	mat4 proj;
	mat4 proj_view;
	vec2 viewport_size;
	float one_pixel_x;
	float one_pixel_y;
	float time;
	uint32_t number_of_lights;
	float rsvd2;
	float rsvd3;

	PigeonWGILight lights[4];
	vec3 ambient;
	float rsvd4;

	float znear;
	float zfar;

	float rsvd0;
	float rsvd1;

	vec3 eye_position;
} PigeonWGISceneUniformData;

void pigeon_wgi_get_normal_model_matrix(const mat4 model, mat4 normal_model_matrix);

void pigeon_wgi_set_object_shadow_mvp_uniform(PigeonWGIDrawObject* data, mat4 model_matrix);
PIGEON_ERR_RET pigeon_wgi_set_uniform_data(PigeonWGISceneUniformData* uniform_data);
