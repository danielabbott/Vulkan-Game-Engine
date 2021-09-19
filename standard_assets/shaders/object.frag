#version 460

layout (constant_id = 0) const bool SC_SSAO = true;
layout (constant_id = 1) const int SC_SHADOW_SAMPLES = 4;

layout(location = 0) in vec3 in_position_model_space;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in flat uint in_draw_call_index;
layout(location = 3) in mat3 in_tangent_to_world;

layout(location = 0) out vec4 out_colour;

layout(binding = 2) uniform sampler2D ssao_image;

layout(binding = 3) uniform sampler2DShadow shadow_maps[4];

layout(binding = 4) uniform sampler2DArray textures[90];

#include "ubo.glsl"

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {

    /* Ambient Occlusion */

    DrawCallObject data = draw_call_objects.obj[in_draw_call_index];  
    vec2 tex_coord = gl_FragCoord.xy / ubo.viewport_size;


    float occlusion = 0;

    if(SC_SSAO) {
        occlusion = texture(ssao_image, tex_coord).r;
    }


    occlusion *= data.ssao_intensity;



    /* Normal map */

    vec3 normal;

    if(data.normal_map_sampler_index_plus1 >= 1) {
        vec2 t = texture(textures[data.normal_map_sampler_index_plus1-1], 
            vec3(in_uv * data.normal_map_uv_base_and_range.zw + data.normal_map_uv_base_and_range.xy,
            data.normal_map_index)).rg * 2 - 1;
        
        normal = in_tangent_to_world * vec3(t.x, t.y, sqrt(1 - t.x*t.x - t.y*t.y));
    }
    else {
        normal = in_tangent_to_world * vec3(0, 0, 1);
    }



    /* Light */

    vec3 colour = ubo.ambient;

	float random_value = rand(tex_coord);

    float theta = random_value * 6.28;

    mat2 rotation_matrix = mat2(
        cos(theta), -sin(theta), 
        sin(theta), cos(theta)
    );

    for(uint i = 0; i < ubo.number_of_lights; i++) {
        Light l = ubo.lights[i];
        float intensity = max(dot(normal, normalize(l.neg_direction)), 0.0);


        if(l.is_shadow_caster != 0.0 && intensity > 0.0) {
            vec4 shadow_xyzw = data.modelViewProj[i+1] * vec4(in_position_model_space, 1.0);

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
                const vec2 spread = vec2(2.3, 1.9);
                for(int j = 0; j < SC_SHADOW_SAMPLES; j++) {
                    vec2 o = rotation_matrix * coordinate_offsets[(i+j + int(random_value*16)) % 16] * spread * shadow_texture_offset;

                    shadow += texture(shadow_maps[i], vec3(shadow_xyzw.xy + o, shadow_xyzw.z)).r;
                }
                intensity *= shadow/float(SC_SHADOW_SAMPLES);             
            }
        }
        colour += intensity*l.light_intensity__and__shadow_pixel_offset.rgb;
    }

    colour *= vec3(1-clamp(occlusion, 0.0, 1));
    colour += vec3(0.1, 0, 0.2) * clamp(occlusion, 0.0, 1)*0.1;

    colour *= data.colour.rgb;

    float alpha = 1;

    if(data.texture_sampler_index_plus1 >= 1) {
        vec2 uv = in_uv * data.texture_uv_base_and_range.zw + data.texture_uv_base_and_range.xy;
        vec4 tex_val = texture(textures[data.texture_sampler_index_plus1-1], vec3(uv, data.texture_index));
        tex_val.rgb *= 1.2;


        if(data.under_colour.a == 1) {
            colour *= mix(data.under_colour.rgb, tex_val.rgb, tex_val.a);
        }
        else {
            colour *= tex_val.rgb;
            if(data.under_colour.a == 2) {
                alpha = tex_val.a;
            }
        }
    }

    colour = max(vec3(0), colour);

    const float fog = 1-smoothstep(0.00005, 0.001, gl_FragCoord.z);
    colour = mix(colour, vec3(4.5,5,5.5), fog); // blend to skybox colour (ish)


    const vec3 luminance_multipliers = vec3(0.2126, 0.7152, 0.0722);
    out_colour = vec4(colour, alpha);
}