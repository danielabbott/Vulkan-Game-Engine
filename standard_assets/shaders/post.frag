#version 450

layout (constant_id = 0) const bool SC_BLOOM = true;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_colour;

layout(binding = 0) uniform sampler2D hdr_render;
layout(binding = 1) uniform sampler2D bloom_texture;


layout(push_constant) uniform PushConstantsObject
{
	vec2 viewport_dimensions;
	vec2 one_pixel; // 1.0 / viewport_dimensions
	float bloom_intensity;
} push_constants;


void main() {
	vec3 bloom;
	
	if(SC_BLOOM) {
		bloom = texture(bloom_texture, gl_FragCoord.xy / push_constants.viewport_dimensions).rgb;
	}

    const vec3 luminance_multipliers = vec3(0.2126, 0.7152, 0.0722);

	vec3 true_colour = texture(hdr_render, in_tex_coord).rgb;

	vec3 colours[4];
	float luminosities[4];

	{
		int i = 0;
		for(float y = -1; y <= 1; y+=2) {
			for(float x = -1; x <= 1; x+=2, i++) {
				colours[i] = texture(hdr_render, in_tex_coord + 0.5*push_constants.one_pixel * vec2(x,y)).rgb;
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
		const float sensitivity = 0.2; // lower = more blur
		// if(abs(luminosities[i] - luminance_average) > sensitivity) {
		// 	is_high_contrast = 1;
		// }
		is_high_contrast += max(sign(abs(luminosities[i] - luminance_average) - sensitivity), 0.0);
	}
	// make is_high_contrast 0 or 1
	is_high_contrast = sign(is_high_contrast);


	// Use blurred colour if contrast is high, otherwise use true value
	vec3 colour = is_high_contrast*blurred_colour + (1-is_high_contrast)*true_colour;

	if(SC_BLOOM) {
		colour += bloom * push_constants.bloom_intensity;
	}



	float luminance = dot(luminance_multipliers, colour);
	
	const float brightness = 0.0;
	const float contrast = 1.2;
	float new_luminance = luminance * contrast + brightness;
	new_luminance /= new_luminance + 1.0;


	colour *= new_luminance / luminance;

	out_colour = vec4(colour, 1.0);
}