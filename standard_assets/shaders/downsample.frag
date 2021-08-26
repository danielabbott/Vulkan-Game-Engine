#version 450

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_colour;

layout(binding = 0) uniform sampler2D src_image;


layout(push_constant) uniform PushConstantsObject
{
	vec2 offset; // UV offset for 1 pixels in src image
    int samples; // 1 = 2x downsample, 2 = 4x, 4 = 8x, 8 = 16x
} push_constants;


void main() {
	vec3 sum = vec3(0);

	// TODO use specialisation constant
	const int samples = push_constants.samples;

	vec2 offset = vec2(0, -push_constants.offset.y * (samples-1));
	for(int y = 0; y < samples; y++) {
		offset.x = -push_constants.offset.x * (samples-1);
		for(int x = 0; x < samples; x++) {
			sum += max(vec3(0), texture(src_image, in_tex_coord + offset).rgb-vec3(10));

			offset.x += push_constants.offset.x*2;
		}

		offset.y += push_constants.offset.y*2;
	}
	
	out_colour = vec4(sum / (samples*samples), 0);
}