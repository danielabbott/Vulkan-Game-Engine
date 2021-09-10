#version 450

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out float out_colour;

layout(binding = 0) uniform sampler2D depth_image;


layout(push_constant) uniform PushConstantsObject
{
	vec2 viewport_dimensions;
	vec2 one_pixel; // 1.0 / viewport_dimensions
	float ssao_cutoff;
} push_constants;


// https://stackoverflow.com/a/4275343/11498001
float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}



void main() {
	float depth = texture(depth_image, vec2(in_tex_coord)).r;// 0 = far, 1 = near

	float occlusion = 0.0;

	if(depth > 0) {
		float depth_right = texture(depth_image, vec2(in_tex_coord) + vec2(push_constants.one_pixel.x, 0)).r;
		float depth_down = texture(depth_image, vec2(in_tex_coord) + vec2(0, push_constants.one_pixel.y)).r;

		vec2 surface_direction = normalize(vec2(depth - depth_right, depth - depth_down));

		const int samples = 8; // TODO specialisation constant
		const vec2 coordinate_offsets[16] = {
			vec2(0.0678041678194117, -0.201228314587562),
			vec2(-0.1470315643725863, -0.23561448942786556),
			vec2(0.0673912238553391, 0.08216908015161421),
			vec2(-0.08953143685348366, -0.1637138143791691),
			vec2(-0.024097381680116083, -0.25873603435891573),
			vec2(0.20687725131395668, -0.1631832773408635),
			vec2(-0.6336758368043167, -0.4296463996108964),
			vec2(0.22061966936261895, -0.5449981869761349),
			vec2(0.5681063572166681, -0.5007253324496278),
			vec2(-0.0970165718482491, 0.12989274996743697),
			vec2(0.08002511555489548, -0.07213557095516908),
			vec2(0.17951370995000035, 0.22067782918838866),
			vec2(0.5236608021246758, 0.8271779047679781),
			vec2(-0.23596494705791657, 0.2543379168604699),
			vec2(0.6420671253801079, 0.12024643256643516),
			vec2(-0.530138178668724, 0.6031239228915898)
		};
		

		float random_value = rand(in_tex_coord);
		float theta = random_value * 6.28;

		mat2 rotation_matrix = mat2(
			cos(theta), -sin(theta), 
			sin(theta), cos(theta)
		);

		float length_random_mul = random_value+0.5;		

		for(int i = 0; i < samples; i++) {
			vec2 p = push_constants.one_pixel * 40 * rotation_matrix * length_random_mul * coordinate_offsets[i];

			// Flip points that are in the wrong direction
			// if(dot(p, surface_direction) < 0) {
			// 	p = -p;
			// }
			p *= sign(min(0, dot(p, surface_direction)))*2 + 1;

			float neighbour_depth = texture(depth_image, in_tex_coord + p).r;

			float delta = neighbour_depth - depth;

			// if(delta > 0 && delta < push_constants.ssao_cutoff) {
			// 	occlusion += smoothstep(0,1,delta/push_constants.ssao_cutoff);
			// }
			occlusion += min(1, sign(push_constants.ssao_cutoff - delta)+1) * smoothstep(0,1,delta/push_constants.ssao_cutoff);
		}
		occlusion /= float(samples);
		occlusion *= 1.6;
		occlusion = clamp(occlusion, 0.0, 1.0);
	}


	out_colour = occlusion;
}