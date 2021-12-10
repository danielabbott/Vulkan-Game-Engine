#version 450

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_colour;

layout(binding = 0) uniform sampler2D hdr_render;


layout(push_constant) uniform PushConstantsObject
{
	vec2 viewport_dimensions;
	vec2 one_pixel; // 1.0 / viewport_dimensions
} push_constants;

void main() {
	const float brightness = 0.0;
	const float contrast = 1.2;

    vec3 colour = texture(hdr_render, in_tex_coord).rgb;

    vec3 colour_left = texture(hdr_render, in_tex_coord + vec2(-push_constants.one_pixel.x, 0)).rgb;
    vec3 colour_right = texture(hdr_render, in_tex_coord + vec2(push_constants.one_pixel.x, 0.0)).rgb;
    vec3 colour_up = texture(hdr_render, in_tex_coord + vec2(0.0, -push_constants.one_pixel.y)).rgb;
    vec3 colour_down = texture(hdr_render, in_tex_coord + vec2(0.0, push_constants.one_pixel.y)).rgb;



    const vec3 luminance_multipliers = vec3(0.2126, 0.7152, 0.0722);

	float luminance_centre = dot(colour, luminance_multipliers);
	float adjusted_luminance_centre = luminance_centre*contrast + brightness;
	adjusted_luminance_centre = adjusted_luminance_centre / (adjusted_luminance_centre + 1.0);

	vec4 surrounding_luminace = vec4(
		dot(colour_left, luminance_multipliers),
		dot(colour_right, luminance_multipliers),
		dot(colour_up, luminance_multipliers),
		dot(colour_down, luminance_multipliers)
	);
	vec4 adjusted_surrounding_luminace = surrounding_luminace*contrast + vec4(brightness);
	adjusted_surrounding_luminace = adjusted_surrounding_luminace / (adjusted_surrounding_luminace + vec4(1.0));

	float centre_luminance_scale = adjusted_luminance_centre / luminance_centre;


	vec4 surrounding_luminance_scales = adjusted_surrounding_luminace / surrounding_luminace;
	colour_left *= surrounding_luminance_scales[0];
	colour_right *= surrounding_luminance_scales[1];
	colour_up *= surrounding_luminance_scales[2];
	colour_down *= surrounding_luminance_scales[3];
	colour *= centre_luminance_scale;

	vec4 luminance_delta = clamp(abs(vec4(adjusted_luminance_centre) - adjusted_surrounding_luminace), 0.0, 1.0);

	bvec4 high_contrast_neighbour = greaterThan(luminance_delta, vec4(0.1));

	float avg_div = 2.0;
	colour += colour;
	if(high_contrast_neighbour[0] || high_contrast_neighbour[1]) {
		avg_div += 2;
		colour += colour_left;
		colour += colour_right;
	}
	if(high_contrast_neighbour[2] || high_contrast_neighbour[3]) {
		avg_div += 2;
		colour += colour_up;
		colour += colour_down;
	}
	colour /= avg_div;

	out_colour = vec4(colour, 1.0);
}