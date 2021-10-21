#version 460

#include "common.glsl"

LOCATION(0) in vec2 pass_tex_coord;
LOCATION(0) out vec4 out_colour;

BINDING(0) uniform sampler2D src_image;

#if __VERSION__ >= 460

// 2 = 4x downsample, 4 = 8x, 8 = 16x
layout (constant_id = 0) const int SC_BLOOM_DOWNSAMPLE_SAMPLES = 8;




layout(push_constant) uniform PushConstantsObject
{
	vec2 offset; // UV offset for 1 pixel in src image
	float min; // default 0 (plain downsample). Set to > 0 for cutoff (for bloom)
} push_constants;

#define OFFSET push_constants.offset
#define MIN push_constants.min

#else

uniform vec3 u_offset_and_min;

#define OFFSET u_offset_and_min.xy
#define MIN u_offset_and_min.z

#endif


void main() {
	vec3 sum = vec3(0);

	vec2 offset = vec2(0, -OFFSET.y * (SC_BLOOM_DOWNSAMPLE_SAMPLES-1));
	for(int y = 0; y < SC_BLOOM_DOWNSAMPLE_SAMPLES; y++) {
		offset.x = -OFFSET.x * (SC_BLOOM_DOWNSAMPLE_SAMPLES-1);
		for(int x = 0; x < SC_BLOOM_DOWNSAMPLE_SAMPLES; x++) {
			sum += max(vec3(0), texture(src_image, pass_tex_coord + offset).rgb - vec3(MIN));

			offset.x += OFFSET.x*2;
		}

		offset.y += OFFSET.y*2;
	}
	
	out_colour = vec4(sum / (SC_BLOOM_DOWNSAMPLE_SAMPLES*SC_BLOOM_DOWNSAMPLE_SAMPLES), 0);
}