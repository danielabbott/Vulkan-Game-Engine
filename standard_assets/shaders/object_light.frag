#version 460

#include "common.glsl"



LOCATION(0) in vec3 pass_position_model_space;
LOCATION(1) in vec2 pass_uv;
LOCATION(2) flat in int pass_draw_index;
LOCATION(3) in vec3 pass_normal;

LOCATION(0) out vec4 out_shadow_values;

BINDING(3) uniform sampler2D depth_image; // opengl binding 5

#if __VERSION__ >= 460

layout (constant_id = 0) const int SC_SSAO_SAMPLES = 8; // 0 to disable
layout (constant_id = 1) const int SC_SHADOW_SAMPLES = 4;
layout (constant_id = 2) const bool SC_FETCH_ALPHA = false;
layout (constant_id = 3) const int SC_COLOUR_COMPONENTS = 2; // 1, 2, or 3


BINDING(4) uniform sampler2DShadow shadow_maps[4];
layout(binding = 5) uniform sampler2DArray textures[59];

#else

uniform sampler2DShadow shadow_map0; // opengl binding 0
uniform sampler2DShadow shadow_map1; // opengl binding 1
uniform sampler2DShadow shadow_map2; // opengl binding 2
uniform sampler2DShadow shadow_map3; // opengl binding 3
uniform sampler2DArray diffuse_texture; // opengl binding 4

#endif


#include "ubo.glsl"
#include "ssao.glsl"

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {

    /* Transparency */

#if __VERSION__ >= 460
    #define data draw_objects.obj[pass_draw_index]
#else
    #define data draw_object.obj
#endif

    float alpha = 1;

    if(SC_FETCH_ALPHA && data.texture_sampler_index_plus1 >= 1.0) {        
        alpha = texture( 
#if __VERSION__ >= 460
            textures[data.texture_sampler_index_plus1-1], 
#else
            diffuse_texture,
#endif
            vec3(pass_uv, data.texture_index)).a;

        if(alpha < 1.0 && data.under_colour.a == 2) {
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


    vec3 normal = pass_normal;

    vec4 output_value; // 0 = full shadow, 1 = no shadow

    /* SSAO */

    float occlusion = 1;
    if(SC_SSAO_SAMPLES > 0) {
        occlusion = 1-get_ssao(tex_coord, random_value, rotation_matrix) * data.ssao_intensity;
        output_value[0] = occlusion;
    }


    /* Lights */
 

    int output_value_index = SC_SSAO_SAMPLES > 0 ? 1 : 0;

    for(int i = 0; i < ubo.number_of_lights; i++) {
        Light l = ubo.lights[i];
        if(l.neg_direction_and_is_shadow_caster.w == 0.0) continue;

        float intensity = max(dot(normal, normalize(l.neg_direction_and_is_shadow_caster.xyz)), 0.0);

        if(intensity <= 0.0) {
            output_value[output_value_index] = 0;
            // output_value[output_value_index] = 1;
            output_value_index++;
            continue;
        }
        
        vec4 shadow_xyzw = data.modelViewProj[i+1] * vec4(pass_position_model_space, 1.0);

        vec2 abs_xy = abs(shadow_xyzw.xy);
        if(abs_xy.x < 1.0 && abs_xy.y < 1.0) {
            // shadow_xyzw.xyz /= shadow_xyzw.w; // Not needed for ortho matrix
            shadow_xyzw.xy = shadow_xyzw.xy*0.5 + vec2(0.5);
            shadow_xyzw.z += 0.0005; // bias

            #ifndef VULKAN
            shadow_xyzw.y = 1-shadow_xyzw.y;
            #endif

            float shadow = 0.0;

            vec2 shadow_texture_offset = vec2(l.light_intensity_and_shadow_pixel_offset.w);
            vec2 spread = vec2(2.3, 1.9);
            for(int j = 0; j < SC_SHADOW_SAMPLES; j++) {
                vec2 o = rotation_matrix * coordinate_offsets[(i+j + int(random_value*16.0)) % 16] * spread * shadow_texture_offset;

#if __VERSION__ >= 460
                shadow += texture(shadow_maps[i], vec3(shadow_xyzw.xy + o, shadow_xyzw.z));
#else
                if(i == 0)
                    shadow += texture(shadow_map0, vec3(shadow_xyzw.xy + o, shadow_xyzw.z));
                else if (i == 1)
                    shadow += texture(shadow_map1, vec3(shadow_xyzw.xy + o, shadow_xyzw.z));
                else if (i == 2)
                    shadow += texture(shadow_map2, vec3(shadow_xyzw.xy + o, shadow_xyzw.z));
                else
                    shadow += texture(shadow_map3, vec3(shadow_xyzw.xy + o, shadow_xyzw.z));
#endif
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
        out_shadow_values = vec4(output_value.rgb, 1.0);
    }
    else if(SC_COLOUR_COMPONENTS == 2) {
        out_shadow_values = vec4(output_value.rg, 0.0, 1.0);
    }
    else {
        out_shadow_values = vec4(output_value.r, 0.0, 0.0, 1.0);
    }
}