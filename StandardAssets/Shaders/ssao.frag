#version 450

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out float out_colour;

layout(binding = 0) uniform sampler2D depth_image;


layout(push_constant) uniform PushConstantsObject
{
	vec2 viewport_dimensions;
	vec2 one_pixel; // 1.0 / viewport_dimensions
	float ssao_parameter;
} push_constants;


// https://stackoverflow.com/a/4275343/11498001
float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}



void main() {
	// TODO SSAO needs some fine-tuning

	float depth0 = texture(depth_image, vec2(in_tex_coord)).r; // 0 = near, 1 = far

	float occlusion = 0.0;

	if(depth0 < 1.0) {

		float depth1 = texture(depth_image, vec2(in_tex_coord) + vec2(push_constants.one_pixel.x, 0.0)).r;
		float depth2 = texture(depth_image, vec2(in_tex_coord) + vec2(0.0, push_constants.one_pixel.y)).r;

		// 50000 found through trial and error :/
		vec2 viewspace_normal = normalize(vec3((depth1-depth0)*50000, (depth2-depth0)*50000, 1)).xy;


		const int samples = 6;
		const vec2 coordinate_offsets[samples] = {
			vec2(0.0678041678194117, -0.201228314587562),
			vec2(-0.1470315643725863, -0.23561448942786556),
			vec2(0.0673912238553391, 0.08216908015161421),
			vec2(-0.08953143685348366, -0.1637138143791691),
			vec2(-0.024097381680116083, -0.25873603435891573),
			vec2(0.20687725131395668, -0.1631832773408635),
			// vec2(-0.6336758368043167, -0.4296463996108964),
			// vec2(0.22061966936261895, -0.5449981869761349),
			// vec2(0.5681063572166681, -0.5007253324496278),
			// vec2(-0.0970165718482491, 0.12989274996743697),
			// vec2(0.08002511555489548, -0.07213557095516908),
			// vec2(0.17951370995000035, 0.22067782918838866),
			// vec2(0.5236608021246758, 0.8271779047679781),
			// vec2(-0.23596494705791657, 0.2543379168604699),
			// vec2(0.6420671253801079, 0.12024643256643516),
			// vec2(-0.530138178668724, 0.6031239228915898),
		};
		

		float random_value = rand(in_tex_coord);
		float theta = random_value * 6.28;

		mat2 rotation_matrix = mat2(
			cos(theta), -sin(theta), 
			sin(theta), cos(theta)
		);

		float length_multiplier = clamp(random_value*2, 0.5, 1.5);

		// sample distance is smaller further away
		float distance_multiplier = clamp((1.0- (depth0-0.95)*20 ), 0.3, 1);

		// AO is lighter further away
		// float intensity_multiplier = pow(1-depth0, 2);//TODO aaghgh
		const float intensity_multiplier = 1.0;

		// Make SSAO parameter bigger for close-up objects
		// Brings out AO detail when looking at things close-up
		float cutoff = push_constants.ssao_parameter * ((pow(clamp(1-depth0, 0.1, 1.0)-0.1, 2)*4)+1);

		for(int i = 0; i < samples; i++) {
			vec2 p = rotation_matrix * length_multiplier * coordinate_offsets[i];



			// Flip one hemicircle
			if(dot(p, viewspace_normal) < 0) {
				p = -p;
			}

			const float radius = 100.0 * distance_multiplier;
			
			p *= push_constants.one_pixel*radius;
			float d = texture(depth_image, in_tex_coord + p).r;

			float delta = depth0-d;

			if(delta > 0 && abs(delta) < cutoff) {
				// occlusion += delta / cutoff;
				occlusion += smoothstep(0.0, 1.0, delta / cutoff);
			}
		}
		occlusion /= float(samples);
		occlusion = clamp(occlusion*intensity_multiplier, 0.0, 1.0);
		occlusion *= 2.2;

	}

	out_colour = occlusion;
}