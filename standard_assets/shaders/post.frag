#version 460

#include "common.glsl"

LOCATION(0) out vec4 out_colour;

BINDING(0) uniform sampler2D hdr_render;
BINDING(1) uniform sampler2D bloom_texture;


#if __VERSION__ >= 460

layout (constant_id = 0) const bool SC_BLOOM = true;

layout(push_constant) uniform PushConstantsObject
{
	vec2 one_pixel; // 1.0 / viewport_dimensions
	float bloom_intensity;
} push_constants;

#define ONE_PIXEL push_constants.one_pixel
#define BLOOM_INTENSITY push_constants.bloom_intensity

#else

uniform vec3 u_one_pixel_and_bloom_intensity;

#define ONE_PIXEL u_one_pixel_and_bloom_intensity.xy
#define BLOOM_INTENSITY u_one_pixel_and_bloom_intensity.z

#endif


void main() {
	vec2 uv = gl_FragCoord.xy * ONE_PIXEL;
	
	vec3 bloom;
	
	if(SC_BLOOM) {
		bloom = texture(bloom_texture, uv).rgb;
	}

    const vec3 luminance_multipliers = vec3(0.299, 0.587, 0.114);

	vec3 true_colour = texture(hdr_render, uv).rgb;
	float luminance_centre = dot(true_colour, luminance_multipliers);

	vec3 colours[4];
	float luminosities[4];

	{
		int i = 0;
		for(float y = -1; y <= 1; y+=2) {
			for(float x = -1; x <= 1; x+=2, i++) {
				colours[i] = texture(hdr_render, uv + 0.5*ONE_PIXEL * vec2(x,y)).rgb;
				luminosities[i] = dot(colours[i], luminance_multipliers);
			}
		}
	}
	
	vec3 blurred_colour = (
		colours[0] +
		colours[1] +
		colours[2] +
		colours[3]
	) / 4;

	float luminance_average = dot(blurred_colour, luminance_multipliers);


	float is_high_contrast = 0;
	for(int i = 0; i < 4; i++) {
		const float sensitivity = 0.15; // lower = more blur
		// if(abs(luminosities[i] - luminance_centre) > sensitivity) {
		// 	is_high_contrast = 1;
		// }
		is_high_contrast += max(sign(abs(luminosities[i] - luminance_centre) - sensitivity), 0.0);
	}
	// make is_high_contrast 0 or 1
	is_high_contrast = sign(is_high_contrast);


	// Use blurred colour if contrast is high, otherwise use true value
	vec3 colour = mix(true_colour, blurred_colour, is_high_contrast);

	if(SC_BLOOM) {
		colour += bloom * BLOOM_INTENSITY;
	}
	

	float luminance = dot(luminance_multipliers, colour);
	float new_luminance = pow(luminance, 1.2); // contrast
	new_luminance = 3.0*new_luminance / (3.0*new_luminance + 1.0); // tone map
	colour *= new_luminance / luminance;

	out_colour = vec4(colour, 1.0);
}
