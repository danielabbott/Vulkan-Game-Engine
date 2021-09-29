#pragma once


typedef struct PigeonWGIRenderConfig {
	bool ssao;
	bool bloom;
	unsigned int shadow_casting_lights; // 0, 1, or 2
	unsigned int shadow_blur_passes;
} PigeonWGIRenderConfig;

