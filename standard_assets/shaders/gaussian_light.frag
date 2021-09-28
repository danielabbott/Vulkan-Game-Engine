#version 450

#ifdef COLOUR_TYPE_RGBA

#define COLOUR_TYPE vec4
#define COLOUR_COMPONENTS rgba

#elif defined(COLOUR_TYPE_RGB)

#define COLOUR_TYPE vec3
#define COLOUR_COMPONENTS rgb

#elif defined(COLOUR_TYPE_R)

#define COLOUR_TYPE float
#define COLOUR_COMPONENTS r

#else

#define COLOUR_TYPE vec2
#define COLOUR_COMPONENTS rg

#endif

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out COLOUR_TYPE out_colour;

layout(binding = 0) uniform sampler2D src_image;
layout(binding = 1) uniform sampler2D depth_buffer;


layout(push_constant) uniform PushConstantsObject
{
	vec2 one_pixel; // 1.0 / viewport_dimensions, one component is 0 (to control blur direction)
	float nearz;
	float farz;
} push_constants;



// 9x9 box guassian filter
// #define NUMBER_OF_WEIGHTS 2
// const float weights[NUMBER_OF_WEIGHTS] = float[](
// 	0.235294118,
// 	0.117647059
// );
// const float centre_weight = 0.294117647;

// 17x17 box guassian filter
// #define NUMBER_OF_WEIGHTS 4
// const float weights[NUMBER_OF_WEIGHTS] = float[](
// 	0.1945945946,
// 	0.1216216216,
// 	0.0540540541,
// 	0.0162162162
// );
// const float centre_weight = 0.2270270270;


// 33x33 guassian filter


#define NUMBER_OF_WEIGHTS 8

const float weights[NUMBER_OF_WEIGHTS] = float[](
	10.0 / 115.0,
	9.0 / 115.0,
	8.0 / 115.0,
	7.0 / 115.0,
	6.0 / 115.0,
	5.0 / 115.0,
	4.0 / 115.0,
	3.0 / 115.0
);
const float centre_weight = 11.0 / 115.0;



// 65x65 box guassian filter

// #define NUMBER_OF_WEIGHTS 16

// const float weights[NUMBER_OF_WEIGHTS] = float[](
// 	16.0 / 289.0,
// 	15.0 / 289.0,
// 	14.0 / 289.0,
// 	13.0 / 289.0,
// 	12.0 / 289.0,
// 	11.0 / 289.0,
// 	10.0 / 289.0,
// 	9.0 / 289.0,
// 	8.0 / 289.0,
// 	7.0 / 289.0,
// 	6.0 / 289.0,
// 	5.0 / 289.0,
// 	4.0 / 289.0,
// 	3.0 / 289.0,
// 	2.0 / 289.0,
// 	1.0 / 289.0
// );
// const float centre_weight = 17.0 / 289.0;

float relinearise_depth(float d) {
	const float nearz = push_constants.farz;
	const float farz = push_constants.nearz;
	return farz*nearz / (-d*(farz - nearz) - farz);
}

void main() {
	// Centre value

	COLOUR_TYPE centre_colour = texture(src_image, in_tex_coord).COLOUR_COMPONENTS;
	COLOUR_TYPE final_colour = centre_colour * centre_weight;
	float centre_depth = relinearise_depth(texture(depth_buffer, in_tex_coord).r);
	
	// Others

	for(float j = -1; j <= 1; j += 2) {
		vec2 offset = j*1.5*push_constants.one_pixel;

		for(int i = 0; i < NUMBER_OF_WEIGHTS; i += 1) {
			vec2 p = in_tex_coord + offset;
			float d = relinearise_depth(texture(depth_buffer, p + push_constants.one_pixel*0.5).r);

			if(abs(d - centre_depth) < 0.07) {
				COLOUR_TYPE c = texture(src_image, p).COLOUR_COMPONENTS;
				final_colour += c * weights[i];
			}
			else {
				final_colour += centre_colour * weights[i];
			}

			offset += j*2*push_constants.one_pixel*1.5;
		}
	}



	out_colour = final_colour;
}