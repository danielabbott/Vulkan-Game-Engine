struct ObjectUniformData {
    vec4 position_min;
    vec4 position_range__and__ssao_intensity;
	mat4 model;
	mat4 modelView;
	mat4 modelViewProj;
	// mat4 normal_model_matrix;
	vec4 colour;
};

// TODO don't use #define for this. will lead to odd buggs
#define position_range position_range__and__ssao_intensity.xyz
#define ssao_intensity position_range__and__ssao_intensity.w

struct Light {
    vec4 neg_direction_and_is_shadow_caster; // a == 0 -> no shadows
    vec4 light_intensity__and__shadow_pixel_offset; // shadow_pixel_offset = 0.5 / resolution
    mat4 shadow_proj_view;
};

// TODO don't use #define for this. will lead to odd buggs
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
    float rsvd2;
    float rsvd3;
    Light lights[4];
    vec3 ambient;
    float rsvd4;

    ObjectUniformData objects[16384];
} ubo;