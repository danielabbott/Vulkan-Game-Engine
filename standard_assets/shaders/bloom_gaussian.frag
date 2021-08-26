#version 450

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_colour;

layout(binding = 0) uniform sampler2D bloom_image;


layout(push_constant) uniform PushConstantsObject
{
	vec2 one_pixel; // 1.0 / viewport_dimensions
} push_constants;


// 9x9 box guassian filter
// #define NUMBER_OF_WEIGHTS 2
// const float weights[NUMBER_OF_WEIGHTS] = float[](
// 	0.235294118,
// 	0.117647059
// );
// const float centre_weight = 0.294117647;

// 17x17 box guassian filter
#define NUMBER_OF_WEIGHTS 4
const float weights[NUMBER_OF_WEIGHTS] = float[](
	0.1945945946,
	0.1216216216,
	0.0540540541,
	0.0162162162
);
const float centre_weight = 0.2270270270;

void do_sample(inout vec3 final_colour, float offset, int i) {
	vec3 a = texture(bloom_image, in_tex_coord + offset*push_constants.one_pixel).rgb;
	final_colour += a * weights[i];
}

void main() {
	// Centre value

	vec3 final_colour = texture(bloom_image, in_tex_coord).rgb * centre_weight;
	
	// Others

	float offset = 1.5;
	for(int i = 0; i < NUMBER_OF_WEIGHTS; i += 1) {
		do_sample(final_colour, offset, i);
		do_sample(final_colour, -offset, i);
		offset += 2;
	}

	out_colour = vec4(final_colour, 0);
}