#version 460

#include "common.glsl"

LOCATION(0) in vec2 pass_tex_coord;
LOCATION(0) out float out_ao;

BINDING(0) uniform sampler2D src_image;

#if __VERSION__ >= 460

// 1 = 2x downscale, 2 = 4x downscale, 4 = 8x, 8 = 16x
layout (constant_id = 0) const int SC_DOWNSCALE_SAMPLES = 8;




layout(push_constant) uniform PushConstantsObject
{
	vec2 offset; // UV offset for 1 pixel in src image
} push_constants;

#define OFFSET push_constants.offset

#else

uniform vec2 OFFSET;

#endif


void main() {
	float ao = 0.0;

	vec2 offset = vec2(0.0, -OFFSET.y * (SC_DOWNSCALE_SAMPLES-1));
	for(int y = 0; y < SC_DOWNSCALE_SAMPLES; y++) {
		offset.x = -OFFSET.x * (SC_DOWNSCALE_SAMPLES-1);
		for(int x = 0; x < SC_DOWNSCALE_SAMPLES; x++) {
			ao += texture(src_image, pass_tex_coord + offset).r;

			offset.x += OFFSET.x*2;
		}

		offset.y += OFFSET.y*2;
	}

	ao /= float(SC_DOWNSCALE_SAMPLES*SC_DOWNSCALE_SAMPLES);

	out_ao = ao * 2.0;
}