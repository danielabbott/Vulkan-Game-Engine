#version 450

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out float out_colour;

layout(binding = 0) uniform sampler2D shadow_image;
layout(binding = 1) uniform sampler2D depth_buffer;


layout(push_constant) uniform PushConstantsObject
{
	vec2 one_pixel; // 1.0 / viewport_dimensions
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

	float y = 1.5;
	for(int i = 0; i < 4; i += 1) {
		for(float j = -1; j <= 1; j += 2) {
			vec2 p = vec2(in_tex_coord.x, in_tex_coord.y + j*y*push_constants.one_pixel.y);
			float d = texture(depth_buffer, p).r;

			// TODO this idea did not work
			if(abs(d0-d) < 0.0045) {
				sum += texture(shadow_image, p).r * weights[i];
			}
			else {
				sum += centre_value * weights[i];
			}
		}
		y += 2;
	}

	out_colour = sum;
}