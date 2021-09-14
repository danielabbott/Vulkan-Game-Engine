#version 460

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


    float occlusion = texture(ssao_image, tex_coord).r;


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

    vec3 worldpos = (data.model * vec4(in_position_model_space, 1)).xyz;
	float random_value = rand(worldpos.xy) * rand(worldpos.yz);

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


                const int samples = 6; // TODO specialisation constant
                const vec2 coordinate_offsets[16] = {
                    vec2(0.0678041678194117, -0.201228314587562),
                    vec2(-0.1470315643725863, -0.23561448942786556),
                    vec2(0.0673912238553391, 0.08216908015161421),
                    vec2(-0.08953143685348366, -0.1637138143791691),
                    vec2(-0.024097381680116083, -0.25873603435891573),
                    vec2(0.20687725131395668, -0.1631832773408635),
                    vec2(-0.6336758368043167, -0.4296463996108964),
                    vec2(0.22061966936261895, -0.5449981869761349),
                    vec2(0.5681063572166681, -0.5007253324496278),
                    vec2(-0.0970165718482491, 0.12989274996743697),
                    vec2(0.08002511555489548, -0.07213557095516908),
                    vec2(0.17951370995000035, 0.22067782918838866),
                    vec2(0.5236608021246758, 0.8271779047679781),
                    vec2(-0.23596494705791657, 0.2543379168604699),
                    vec2(0.6420671253801079, 0.12024643256643516),
                    vec2(-0.530138178668724, 0.6031239228915898)
                };

                float shadow = 0;

                const vec2 shadow_texture_offset = vec2(l.light_intensity__and__shadow_pixel_offset.w);
                const float spread = 1.0;
                for(int j = 0; j < samples; j++) {
                    vec2 o = rotation_matrix * coordinate_offsets[(i+j + int(random_value*16)) % 16] * shadow_texture_offset * spread;

                    shadow += texture(shadow_maps[i], vec3(shadow_xyzw.xy + o, shadow_xyzw.z)).r;
                }
                intensity *= shadow/float(samples);                
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