#version 460

#include "common.glsl"

#ifdef COLOUR_TYPE_RGBA

#define COLOUR_TYPE vec4
#define COLOUR_COMPONENTS rgba
#define COLOUR_0 vec4(0.0,0.0,0.0,0.0)

#elif defined(COLOUR_TYPE_RGB)

#define COLOUR_TYPE vec3
#define COLOUR_COMPONENTS rgb
#define COLOUR_0 vec3(0.0,0.0,0.0)

#elif defined(COLOUR_TYPE_R)

#define COLOUR_TYPE float
#define COLOUR_COMPONENTS r
#define COLOUR_0 0.0

#else

#define COLOUR_TYPE vec2
#define COLOUR_COMPONENTS rg
#define COLOUR_0 vec2(0.0,0.0)

#endif

LOCATION(0) in vec2 pass_tex_coord;

LOCATION(0) out COLOUR_TYPE out_colour;

BINDING(0) uniform sampler2D src_image;
BINDING(1) uniform sampler2D depth_buffer;

#if __VERSION__ >= 460

layout(push_constant) uniform PushConstantsObject
{
	vec2 sample_distance; // 0.5 / viewport_dimensions for 3x3 blur
	vec2 half_pixel;// 0.5 / viewport_dimensions
	float nearz;
	float farz;
} push_constants;

#define SAMPLE_DISTANCE push_constants.sample_distance
#define HALF_PIXEL push_constants.half_pixel
#define NEARZ push_constants.farz
#define FARZ push_constants.nearz

#else

uniform vec4 u_dist_and_half;
uniform vec2 u_near_far;

#define SAMPLE_DISTANCE u_dist_and_half.rg
#define HALF_PIXEL u_dist_and_half.ba
#define NEARZ u_near_far.y
#define FARZ u_near_far.x

#endif



float relinearise_depth(float d) {
	return FARZ*NEARZ / (-d*(FARZ - NEARZ) - FARZ);
}

void main() {
	float centre_depth = relinearise_depth(texture(depth_buffer, pass_tex_coord).r);

	COLOUR_TYPE colour = COLOUR_0;
	float average_sum = 0.0;
	
	for(int y = -1; y <= 1; y += 2) {
		for(int x = -1; x <= 1; x += 2) {
			vec2 xy = vec2(x,y);
			vec2 p = pass_tex_coord + xy * SAMPLE_DISTANCE;
			float d = relinearise_depth(texture(depth_buffer, p + xy*HALF_PIXEL).r);

			COLOUR_TYPE c = texture(src_image, p).COLOUR_COMPONENTS;

			if(abs(d - centre_depth) < 0.07) {
				colour += c;
				average_sum += 1;
			}
		}
	}

	if(average_sum != 0.0) {
		colour /= average_sum;
	}
	else {
		colour = texture(src_image, pass_tex_coord).COLOUR_COMPONENTS;
	}

	out_colour = colour;
}