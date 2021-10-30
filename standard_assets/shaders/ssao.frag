#version 460

#include "common.glsl"
#include "random.glsl"

#if __VERSION__ >= 460
layout (constant_id = 0) const int SC_SSAO_SAMPLES = 3;


layout(push_constant) uniform PushConstantsObject
{
	float nearz, farz;
	vec2 one_pixel;
	float ssao_cutoff;
} push_constants;

#define FARZ push_constants.nearz
#define NEARZ push_constants.farz
#define CUTOFF push_constants.ssao_cutoff
#define ONE_PIXEL push_constants.one_pixel

#else

uniform vec3 u_near_far_cutoff;
uniform vec2 ONE_PIXEL;

#define FARZ u_near_far_cutoff.x
#define NEARZ u_near_far_cutoff.y
#define CUTOFF u_near_far_cutoff.z

#endif

LOCATION(0) in vec2 pass_tex_coord;
LOCATION(0) out float out_ao;

BINDING(0) uniform sampler2D depth_image;


float relinearise_depth(float d) {
	return FARZ*NEARZ / (-d*(FARZ - NEARZ) - FARZ);
}

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}


void main() {
	float random_value = rand(pass_tex_coord);

	float theta = random_value * 6.28;

	mat2 rotation_matrix = mat2(
		cos(theta), -sin(theta),
		sin(theta), cos(theta)
	);

	float depth = relinearise_depth(texture(depth_image, vec2(pass_tex_coord)).r);// 0 = near, 1 = far

	if(depth == 0.0) {
		out_ao = 0.0;
		return;
	}


	float depth_right = relinearise_depth(texture(depth_image, vec2(pass_tex_coord) + vec2(ONE_PIXEL.x, 0)).r);
	float depth_down = relinearise_depth(texture(depth_image, vec2(pass_tex_coord) + vec2(0, ONE_PIXEL.y)).r);

	vec2 surface_direction = normalize(vec2(depth_right - depth, depth_down-depth));
	
	float length_random_mul = random_value+0.5;

	float occlusion = 0.0;

	for(int i = 0; i < SC_SSAO_SAMPLES; i++) {
		vec2 p = vec2(ONE_PIXEL.x, ONE_PIXEL.y) * 20 * rotation_matrix
			* length_random_mul * coordinate_offsets[(i + int(random_value*16.0)) % 16];

		// Flip points that are in the wrong direction
		if(dot(p, surface_direction) < 0.0) {
			p = -p;
		}
		// p *= sign(min(0, dot(p, surface_direction)))*2 + 1;

		float neighbour_depth = relinearise_depth(texture(depth_image, pass_tex_coord + p).r);

		float delta = depth - neighbour_depth;

		if(delta > 0.001 && delta < CUTOFF) {
			occlusion += smoothstep(0.0,1.0, delta / CUTOFF);
		}
		// occlusion += min(1.0, sign(CUTOFF - delta)+1.0) * smoothstep(0.0,1.0, delta / CUTOFF);
	}
	occlusion /= float(SC_SSAO_SAMPLES);
	occlusion = clamp(occlusion*2.0, 0.0, 1.0);
	


	out_ao = occlusion;
}

