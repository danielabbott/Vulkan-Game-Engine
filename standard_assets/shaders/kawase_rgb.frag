#version 460

#include "common.glsl"

LOCATION(0) in vec2 pass_tex_coord;

LOCATION(0) out vec3 out_colour;

BINDING(0) uniform sampler2D src_image;

#if __VERSION__ >= 460

layout(push_constant) uniform PushConstantsObject
{
	vec2 sample_distance;
} push_constants;

#define SAMPLE_DISTANCE push_constants.sample_distance

#else

uniform vec2 SAMPLE_DISTANCE;

#endif


void main() {
	vec3 final_colour = vec3(0.0);
	
	for(int y = -1; y <= 1; y += 2) {
		for(int x = -1; x <= 1; x += 2) {
			vec2 p = pass_tex_coord + vec2(x,y) * SAMPLE_DISTANCE;

			final_colour += texture(src_image, p).rgb;
		}
	}

	out_colour = final_colour * 0.25;
}