#pragma once

#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <cglm/types.h>
#include <pigeon/array_list.h>

struct PigeonComponent;

typedef enum {
	PIGEON_TRANSFORM_TYPE_NONE, // transform is same as parent (or world origin for root)
	PIGEON_TRANSFORM_TYPE_MATRIX,
	PIGEON_TRANSFORM_TYPE_SRT,
} PigeonTransformType;

typedef struct PigeonTransform {
	struct PigeonTransform* parent; // may be NULL

	// call pigeon_invalidate_world_transform() after editing
	PigeonTransformType transform_type;
	union {
		mat4 matrix_transform;
		struct {
			vec3 scale;
			vec4 rotation;
			vec3 translation;
		};
	};

	mat4 world_transform_cache;
	bool world_transform_cached;

	PigeonArrayList* children; // array of PigeonTransform*
	PigeonArrayList* components; // array of PigeonComponent*
} PigeonTransform;

// ** Transform objects are allocated from pools, always use these functions for creation/destruction

// parent can be NULL
PigeonTransform* pigeon_create_transform(PigeonTransform* parent);
void pigeon_destroy_transform(PigeonTransform*);

PIGEON_ERR_RET pigeon_join_transform_and_component(PigeonTransform*, struct PigeonComponent*);
void pigeon_unjoin_transform_and_component(PigeonTransform*, struct PigeonComponent*);

void pigeon_invalidate_world_transform(PigeonTransform*);
void pigeon_scene_calculate_world_matrix(PigeonTransform*);
