#version 450

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_colour;

layout(binding = 0) uniform sampler2D hdr_render;
layout(binding = 1) uniform sampler2D bloom_texture;


layout(push_constant) uniform PushConstantsObject
{
	vec2 viewport_dimensions;
	vec2 one_pixel; // 1.0 / viewport_dimensions
} push_constants;

void main() {
	vec3 bloom = texture(bloom_texture, gl_FragCoord.xy / push_constants.viewport_dimensions).rgb;

    const vec3 luminance_multipliers = vec3(0.2126, 0.7152, 0.0722);

	vec3 colours[9];
	float luminosities[9];
	float luminance_average = 0;

	int i = 0;
	for(float y = -1; y <= 1; y++) {
		for(float x = -1; x <= 1; x++, i++) {
			vec4 colour_and_lum = texture(hdr_render, in_tex_coord + push_constants.one_pixel * vec2(x,y));
			colours[i] = colour_and_lum.rgb;
			luminosities[i] = colour_and_lum.a;
			luminance_average += colour_and_lum.a;			
		}
	}


	luminance_average /= 9;

	float is_high_contrast = 0;
	for(i = 0; i < 9; i++) {
		const float sensitivity = 0.2; // lower = more blur
		// if(abs(luminosities[i] - luminance_average) > sensitivity) {
		// 	is_high_contrast = 1;
		// }
		is_high_contrast += max(sign(abs(luminosities[i] - luminance_average) - sensitivity), 0.0);
	}

	const float brightness = 0.0;
	const float contrast = 1.2;

	// float luminance = luminance_average * contrast + brightness;
	// luminance /= luminance + 1.0;
	
	vec3 colour;

	if(is_high_contrast > 0) {
		colour = 
		(
			colours[0] +
			colours[1]*2 +
			colours[2] +
			colours[3]*2 +
			colours[4]*3 +
			colours[5]*2 +
			colours[6] +
			colours[7]*2 +
			colours[8]
		) / 15;

		// colour=vec3(11,0,0); // Uncomment to highlight edges
	}
	else {
		colour = colours[4];
	}

	colour += bloom*1.5;

	float luminance = dot(luminance_multipliers, colour);
	
	float new_luminance = luminance * contrast + brightness;
	new_luminance /= new_luminance + 1.0;


	colour *= new_luminance / luminance;



	out_colour = vec4(colour, 1.0);
}