#ifndef _UBO_GLSL_
#define _UBO_GLSL_

struct Light {
    vec4 neg_direction_and_is_shadow_caster; // a == 0 -> no shadows
    vec4 light_intensity__and__shadow_pixel_offset; // shadow_pixel_offset = 0.5 / resolution
    mat4 shadow_proj_view;
};

#define neg_direction neg_direction_and_is_shadow_caster.xyz
#define is_shadow_caster neg_direction_and_is_shadow_caster.w

layout(binding = 0, std140) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec2 viewport_size;
    float one_pixel_x;
    float one_pixel_y;
    float time;
    uint number_of_lights;
	float ssao_cutoff;
    float rsvd3;
    Light lights[4];
    vec3 ambient;
    float rsvd4;
    float znear;
    float zfar;
} ubo;



struct DrawCallObject {
    vec4 position_min;
    vec4 position_range__and__ssao_intensity;
	mat4 modelViewProj[5];
    
	mat4 model;
    mat4 normal_model_matrix;
	mat4 modelView;

    vec4 colour;
    vec4 under_colour;
    vec4 texture_uv_base_and_range; // .xy = base, .zw = range
    vec4 normal_map_uv_base_and_range;

    uint texture_sampler_index_plus1; // into array of glsl samplers
    float texture_index; // into array texture

    uint normal_map_sampler_index_plus1;
    float normal_map_index;
};

#define position_range position_range__and__ssao_intensity.xyz
#define ssao_intensity position_range__and__ssao_intensity.w

layout(binding = 1, std430) readonly restrict buffer DrawCallsObject {
    DrawCallObject obj[64];
} draw_call_objects;

#endif