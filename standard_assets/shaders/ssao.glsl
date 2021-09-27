#ifndef _SSAO_GLSL_
#define _SSAO_GLSL_

#include "random.glsl"
#include "ubo.glsl"

float relinearise_depth(float d) {
	const float nearz = ubo.zfar;
	const float farz = ubo.znear;
	return farz*nearz / (-d*(farz - nearz) - farz);
}

float get_ssao(vec2 tex_coord, float random_value, mat2 rotation_matrix) {
	if(SC_SSAO_SAMPLES == 0 || ubo.ssao_cutoff <= 0) return 0;

	float depth = relinearise_depth(texture(depth_image, vec2(tex_coord)).r);// 0 = near, 1 = far


	float depth_right = relinearise_depth(texture(depth_image, vec2(tex_coord) + vec2(ubo.one_pixel_x, 0)).r);
	float depth_down = relinearise_depth(texture(depth_image, vec2(tex_coord) + vec2(0, ubo.one_pixel_y)).r);

	vec2 surface_direction = normalize(vec2(depth_right - depth, depth_down-depth));
	
	float length_random_mul = random_value+0.5;

	float occlusion = 0.0;

	for(int i = 0; i < SC_SSAO_SAMPLES; i++) {
		vec2 p = vec2(ubo.one_pixel_x, ubo.one_pixel_y) * 20 * rotation_matrix
			* length_random_mul * coordinate_offsets[(i + int(random_value*16)) % 16];

		// Flip points that are in the wrong direction
		if(dot(p, surface_direction) < 0) {
			p = -p;
		}
		// p *= sign(min(0, dot(p, surface_direction)))*2 + 1;

		float neighbour_depth = relinearise_depth(texture(depth_image, tex_coord + p).r);

		float delta = depth - neighbour_depth;

		if(delta > 0.001 && delta < ubo.ssao_cutoff) {
			occlusion += smoothstep(0,1,delta/ubo.ssao_cutoff);
		}
		// occlusion += min(1, sign(ubo.ssao_cutoff - delta)+1) * smoothstep(0,1,delta/ubo.ssao_cutoff);
	}
	occlusion /= float(SC_SSAO_SAMPLES);
	occlusion = clamp(occlusion*2, 0.0, 1.0);
	


	return occlusion;
}

#endif
