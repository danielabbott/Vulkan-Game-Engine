#version 460


layout (constant_id = 0) const int SC_SSAO_SAMPLES = 8; // 0 to disable
layout (constant_id = 1) const int SC_SHADOW_SAMPLES = 4;
layout (constant_id = 2) const bool SC_FETCH_ALPHA = false;
layout (constant_id = 3) const int SC_COLOUR_COMPONENTS = 2; // 1, 2, or 3

layout(location = 0) in vec3 in_position_model_space;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in flat uint in_draw_call_index;
layout(location = 3) in vec3 in_normal;

layout(location = 0) out vec4 out_shadow_values;


layout(binding = 2) uniform sampler2D depth_image;

layout(binding = 3) uniform sampler2DShadow shadow_maps[4];

layout(binding = 4) uniform sampler2DArray textures[90];


#include "ubo.glsl"
#include "ssao.glsl"

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {

    /* Transparency */

    DrawCallObject data = draw_call_objects.obj[in_draw_call_index]; 
    float alpha = 1;

    if(SC_FETCH_ALPHA && data.texture_sampler_index_plus1 >= 1) {
        vec2 uv = in_uv * data.texture_uv_base_and_range.zw + data.texture_uv_base_and_range.xy;
        alpha = texture(textures[data.texture_sampler_index_plus1-1], vec3(uv, data.texture_index)).a;

        if(alpha < 1.0) {
            discard;
        }
    }

    /**/


    vec2 tex_coord = gl_FragCoord.xy / ubo.viewport_size;

	float random_value = rand(tex_coord);

    float theta = random_value * 6.28;

    mat2 rotation_matrix = mat2(
        cos(theta), -sin(theta), 
        sin(theta), cos(theta)
    );


    vec3 normal = in_normal;

    vec4 output_value; // 0 = full shadow, 1 = no shadow

    /* SSAO */

    float occlusion = 1;
    if(SC_SSAO_SAMPLES > 0) {
        occlusion = 1-get_ssao(tex_coord, random_value, rotation_matrix) * data.ssao_intensity;
        output_value[0] = occlusion;
    }


    /* Lights */
 

    uint output_value_index = SC_SSAO_SAMPLES > 0 ? 1 : 0;


    for(uint i = 0; i < ubo.number_of_lights; i++) {
        Light l = ubo.lights[i];
        if(l.is_shadow_caster == 0.0) continue;

        float intensity = max(dot(normal, normalize(l.neg_direction)), 0.0);

        if(intensity <= 0.0) {
            output_value[output_value_index] = 0;
            // output_value[output_value_index] = 1;
            output_value_index++;
            continue;
        }
        
        vec4 shadow_xyzw = data.modelViewProj[i+1] * vec4(in_position_model_space, 1.0);

        vec2 abs_xy = abs(shadow_xyzw.xy);
        if(abs_xy.x < 1.0 && abs_xy.y < 1.0) {
            // shadow_xyzw.xyz /= shadow_xyzw.w; // Not needed for ortho matrix
            shadow_xyzw.xy = shadow_xyzw.xy*0.5 + vec2(0.5);
            shadow_xyzw.z += 0.0001; // bias

            float shadow = 0;

            const vec2 shadow_texture_offset = vec2(l.light_intensity__and__shadow_pixel_offset.w);
            const vec2 spread = vec2(2.3, 1.9);
            for(int j = 0; j < SC_SHADOW_SAMPLES; j++) {
                vec2 o = rotation_matrix * coordinate_offsets[(i+j + int(random_value*16)) % 16] * spread * shadow_texture_offset;

                shadow += texture(shadow_maps[i], vec3(shadow_xyzw.xy + o, shadow_xyzw.z)).r;
            }
            shadow /= float(SC_SHADOW_SAMPLES);  

            output_value[output_value_index] = intensity * shadow;  
        }
        else {
            output_value[output_value_index] = intensity;
            // output_value[output_value_index] = 1;
        }
        
        output_value_index++;
    }

    if(SC_COLOUR_COMPONENTS == 4) {
        out_shadow_values = output_value;
    }
    else if(SC_COLOUR_COMPONENTS == 3) {
        out_shadow_values = vec4(output_value.rgb, 1);
    }
    else if(SC_COLOUR_COMPONENTS == 2) {
        out_shadow_values = vec4(output_value.rg, 0, 1);
    }
    else {
        out_shadow_values = vec4(output_value.r, 0, 0, 1);
    }
}