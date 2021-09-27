#version 450

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec3 out_colour;

layout(binding = 0) uniform sampler2D src_image;


layout(push_constant) uniform PushConstantsObject
{
	vec2 one_pixel; // 1.0 / viewport_dimensions, one component is 0 (to control blur direction)
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

void main() {
	// Centre value

	vec3 final_colour = texture(src_image, in_tex_coord).rgb * centre_weight;
	
	// Others

	vec2 offset = 1.5*push_constants.one_pixel;
	for(int i = 0; i < NUMBER_OF_WEIGHTS; i += 1) {
		for(float j = -1; j <= 1; j += 2) {
			final_colour += texture(src_image, in_tex_coord + j*offset).rgb * weights[i];
		}
		offset += 2*push_constants.one_pixel;
	}

	out_colour = final_colour;
}