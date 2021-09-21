#version 460

layout (constant_id = 0) const int SC_SHADOW_SAMPLES = 4;

layout(location = 0) in vec3 in_position_model_space;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in flat uint in_draw_call_index;
layout(location = 3) in mat3 in_tangent_to_world;

layout(location = 0) out float out_colour;

layout(binding = 2) uniform sampler2DShadow shadow_map;

#include "ubo.glsl"

layout(push_constant) uniform PushConstantsObject
{
	uint draw_call_index_offset;
    uint light_index;
} push_constants;

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    

    DrawCallObject data = draw_call_objects.obj[in_draw_call_index];  


    uint light_index = push_constants.light_index;

    vec2 tex_coord = gl_FragCoord.xy / ubo.viewport_size;
	float random_value = rand(tex_coord);

    float theta = random_value * 6.28;

    mat2 rotation_matrix = mat2(
        cos(theta), -sin(theta), 
        sin(theta), cos(theta)
    );

    Light l = ubo.lights[light_index];

    out_colour = 1;

    vec4 shadow_xyzw = data.modelViewProj[light_index+1] * vec4(in_position_model_space, 1.0);

    vec2 abs_xy = abs(shadow_xyzw.xy);
    if(abs_xy.x < 1.0 && abs_xy.y < 1.0) {
        // shadow_xyzw.xyz /= shadow_xyzw.w; // Not needed for ortho matrix
        shadow_xyzw.xy = shadow_xyzw.xy*0.5 + vec2(0.5);
        shadow_xyzw.z -= 0.001; // bias


        const vec2 coordinate_offsets[16] = {
            vec2(0.4875777897915544, -0.5842024029676136),
            vec2(-0.3648700731469788, 0.056668034376749894),
            vec2(-0.12724161595106415, -0.09903190311673399),
            vec2(0.0375108787461495, -0.10743292521119407),
            vec2(-0.459155057942989, -0.2606492924060484),
            vec2(0.25521206628842774, 0.659339368524322),
            vec2(-0.5174528744502028, -0.8094710682528433),
            vec2(-0.03084910851411543, -0.33607143213936747),
            vec2(-0.41634260498132647, 0.20638500309903726),
            vec2(0.5427206636967147, 0.3225632302330816),
            vec2(0.17582566711558972, 0.32760362406125637),
            vec2(-0.3707020801065227, -0.7597524167803934),
            vec2(-0.6105668193569544, 0.7612826982853885),
            vec2(0.6155144516286416, -0.12755274266548586),
            vec2(-0.023345694378709314, 0.7177457877789012),
            vec2(0.14610311618695426, -0.35999266309384187),
        };

        float shadow = 0;

        const vec2 shadow_texture_offset = vec2(l.light_intensity__and__shadow_pixel_offset.w);
        const vec2 spread = vec2(1, 0.8)*2.2;
        for(int j = 0; j < SC_SHADOW_SAMPLES; j++) {
            vec2 o = rotation_matrix * coordinate_offsets[(light_index+j + int(random_value*16)) % 16] * spread * shadow_texture_offset;

            shadow += texture(shadow_map, vec3(shadow_xyzw.xy + o, shadow_xyzw.z)).r;
        }
        out_colour = shadow/float(SC_SHADOW_SAMPLES);             
    }
    
}