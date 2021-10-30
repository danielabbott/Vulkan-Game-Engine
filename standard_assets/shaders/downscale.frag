#version 460

#include "common.glsl"

LOCATION(0) in vec2 pass_tex_coord;
LOCATION(0) out vec4 out_colour;

BINDING(0) uniform sampler2D src_image;

#if __VERSION__ >= 460

// 1 = 2x downscale, 2 = 4x downscale, 4 = 8x, 8 = 16x
layout (constant_id = 0) const int SC_DOWNSCALE_SAMPLES = 8;
layout (constant_id = 1) const bool SC_HIGH_PASS_FILTER = true;




layout(push_constant) uniform PushConstantsObject
{
	vec2 offset; // UV offset for 1 pixel in src image
	float min; // default 0 (plain downscale). Set to > 0 for cutoff (for bloom)
} push_constants;

#define OFFSET push_constants.offset
#define MIN push_constants.min

#else

#ifndef SC_HIGH_PASS_FILTER
#define SC_HIGH_PASS_FILTER true
#endif

uniform vec3 u_offset_and_min;

#define OFFSET u_offset_and_min.xy
#define MIN u_offset_and_min.z

#endif

const vec3 luminance_multipliers = vec3(0.299, 0.587, 0.114);

void main() {
	vec3 c = vec3(0);

	vec2 offset = vec2(0, -OFFSET.y * (SC_DOWNSCALE_SAMPLES-1));
	for(int y = 0; y < SC_DOWNSCALE_SAMPLES; y++) {
		offset.x = -OFFSET.x * (SC_DOWNSCALE_SAMPLES-1);
		for(int x = 0; x < SC_DOWNSCALE_SAMPLES; x++) {
			c += texture(src_image, pass_tex_coord + offset).rgb;

			offset.x += OFFSET.x*2;
		}

		offset.y += OFFSET.y*2;
	}

	c /= float(SC_DOWNSCALE_SAMPLES*SC_DOWNSCALE_SAMPLES);
	
	float intensity = dot(c, luminance_multipliers);

	if(!SC_HIGH_PASS_FILTER || intensity >= MIN)
		out_colour = vec4(c, 0);
	else
		out_colour = vec4(0.0);

}