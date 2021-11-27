#pragma once

struct PigeonArrayList;

typedef enum {
	PIGEON_COMPONENT_TYPE_NONE,
	PIGEON_COMPONENT_TYPE_MATERIAL_RENDERER,
	PIGEON_COMPONENT_TYPE_LIGHT,
	PIGEON_COMPONENT_TYPE_AUDIO_PLAYER,
} PigeonComponentType;

typedef struct PigeonComponent {
	struct PigeonArrayList* transforms; // array of PigeonTransform*
	PigeonComponentType type;
	int users; // TODO
} PigeonComponent;
