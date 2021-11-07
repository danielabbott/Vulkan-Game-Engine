#ifndef _UBO_GLSL_
#define _UBO_GLSL_

#define LIGHT_TYPE_DIRECTIONAL 0.0
#define LIGHT_TYPE_POINT 1.0 // does not support shadows

struct Light {
    vec4 world_pos_and_type;
    vec4 neg_direction_and_is_shadow_caster; // a == 0 -> no shadows
    vec4 light_intensity_and_shadow_pixel_offset; // shadow_pixel_offset = 0.5 / resolution
    mat4 shadow_proj_view;
};



#if __VERSION__ >= 460
layout(binding = 0, std140)
#else
layout(std140)
#endif
uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec2 viewport_size;
    vec2 one_pixel;
    float time;
    int number_of_lights;
	float ssao_cutoff;
    float rsvd3;
    Light lights[4];
    vec4 ambient;
    float znear;
    float zfar;
    float rsvd0;
    float rsvd1;
    vec3 eye_position;
} ubo;

#ifndef NO_DRAW_OBJECTS

struct DrawObjectDepthOnly {
	mat4 modelViewProj;
	mat4 model;
    vec4 position_min;
    vec4 position_range;
};

struct DrawObject {
	mat4 modelViewProj[5];
    
	mat4 model;
    mat4 normal_model_matrix;

    vec4 position_min;
    vec4 position_range_and_ssao_intensity;

    int texture_sampler_index_plus1; // into array of glsl samplers
    float texture_index; // into array texture
    
    int first_bone_index;

    float specular_intensity;

    vec4 colour; // colour components should be in [0,1], alpha = luminosity
    vec4 under_colour;

    int normal_map_sampler_index_plus1;
    float normal_map_index;
    
    int rsvd1;
    int rsvd2;
};

#define position_range position_range_and_ssao_intensity.xyz
#define ssao_intensity position_range_and_ssao_intensity.w


#if __VERSION__ >= 460

    layout(binding = 1, std140) readonly restrict buffer DrawObjectSSBO {
        DrawObject obj[];
    } draw_objects;

#else


    layout(std140) uniform DrawObjectUniform {
        DrawObject obj;
    } draw_object;

#endif

#endif


#endif