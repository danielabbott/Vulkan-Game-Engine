#version 450

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out float out_colour;

layout(binding = 0) uniform sampler2D ssao_image;


layout(push_constant) uniform PushConstantsObject
{
	vec2 viewport_dimensions;
	vec2 one_pixel; // 1.0 / viewport_dimensions
} push_constants;


// 17x17 box guassian filter


const float weights[4] = float[](
	0.1945945946,
	0.1216216216,
	0.0540540541,
	0.0162162162
);

// const float weights[2] = float[](
// 	0.1945945946+0.1216216216,
// 	0.0540540541+0.0162162162
// );

const float centre_weight = 0.2270270270;

void main() {
	// Centre value

	float sum = texture(ssao_image, in_tex_coord).r * centre_weight;
	
	// Others

	float y = 1.5;
	for(int i = 0; i < 4; i += 1) {
		sum += texture(ssao_image, vec2(in_tex_coord.x, in_tex_coord.y + y*push_constants.one_pixel.y)).r
			* weights[i];
		sum += texture(ssao_image, vec2(in_tex_coord.x, in_tex_coord.y - y*push_constants.one_pixel.y)).r
			* weights[i];
		y += 2;
	}

	out_colour = sum;
}