#version 450

// 2 = 4x downsample, 4 = 8x, 8 = 16x
layout (constant_id = 0) const int SC_BLOOM_DOWNSAMPLE_SAMPLES = 8;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_colour;

layout(binding = 0) uniform sampler2D src_image;


layout(push_constant) uniform PushConstantsObject
{
	vec2 offset; // UV offset for 1 pixel in src image
	float min; // default 0 (plain downsample). Set to > 0 for cutoff (for bloom)
} push_constants;


void main() {
	vec3 sum = vec3(0);

	vec2 offset = vec2(0, -push_constants.offset.y * (SC_BLOOM_DOWNSAMPLE_SAMPLES-1));
	for(int y = 0; y < SC_BLOOM_DOWNSAMPLE_SAMPLES; y++) {
		offset.x = -push_constants.offset.x * (SC_BLOOM_DOWNSAMPLE_SAMPLES-1);
		for(int x = 0; x < SC_BLOOM_DOWNSAMPLE_SAMPLES; x++) {
			sum += max(vec3(0), texture(src_image, in_tex_coord + offset).rgb-vec3(push_constants.min));

			offset.x += push_constants.offset.x*2;
		}

		offset.y += push_constants.offset.y*2;
	}
	
	out_colour = vec4(sum / (SC_BLOOM_DOWNSAMPLE_SAMPLES*SC_BLOOM_DOWNSAMPLE_SAMPLES), 0);
}