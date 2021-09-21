#version 450

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out float out_colour;

layout(binding = 0) uniform sampler2D shadow_image;
layout(binding = 1) uniform sampler2D depth_buffer;


layout(push_constant) uniform PushConstantsObject
{
	vec2 one_pixel; // 1.0 / viewport_dimensions, x*=2
} push_constants;


// 17x17 box guassian filter


const float weights[4] = float[](
	0.1945945946,
	0.1216216216,
	0.0540540541,
	0.0162162162
);

const float centre_weight = 0.2270270270;

void main() {
	// Centre value

	float centre_value = texture(shadow_image, in_tex_coord).r;
	float d0 = texture(depth_buffer, in_tex_coord).r;
	
	float sum = centre_value * centre_weight;
	
	// Others

	float x = 1.5;
	for(int i = 0; i < 4; i += 1) {
		for(float j = -1; j <= 1; j += 2) {
			float d = texture(depth_buffer, 
				vec2((in_tex_coord.x + j*x*push_constants.one_pixel.x)*0.5, in_tex_coord.y)).r;

			if(abs(d0-d) < 0.0045) {
				sum += texture(shadow_image, vec2(in_tex_coord.x + j*x*push_constants.one_pixel.x, in_tex_coord.y)).r
					* weights[i];
			}
			else {
				sum += centre_value * weights[i];
			}
		}
		x += 2;
	}

	out_colour = sum;
}