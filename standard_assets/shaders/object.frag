#version 460

layout (constant_id = 0) const bool SC_SSAO_ENABLED = true;


layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in flat uint in_draw_call_index;
layout(location = 3) in mat3 in_tangent_to_world;

layout(location = 0) out vec4 out_colour;


layout(binding = 2) uniform sampler2D shadow_texture;

layout(binding = 4) uniform sampler2DArray textures[90];

#include "ubo.glsl"


void main() {
    DrawCallObject data = draw_call_objects.obj[in_draw_call_index];  
    vec2 tex_coord = gl_FragCoord.xy / ubo.viewport_size;

    vec3 colour = ubo.ambient;
    vec4 shadow_values = texture(shadow_texture, tex_coord);

    /* Normal map */

    vec3 normal = in_normal;
    vec3 normal_detailed;

    if(data.normal_map_sampler_index_plus1 >= 1) {
        vec2 t = texture(textures[data.normal_map_sampler_index_plus1-1], 
            vec3(in_uv * data.normal_map_uv_base_and_range.zw + data.normal_map_uv_base_and_range.xy,
            data.normal_map_index)).rg * 2 - 1;
        
        normal_detailed = in_tangent_to_world * vec3(t.x, t.y, sqrt(1 - t.x*t.x - t.y*t.y));
    }
    else {
        normal_detailed = normal;
    }

    /* light */

    uint shadow_values_index = SC_SSAO_ENABLED ? 1 : 0;
    for(uint i = 0; i < ubo.number_of_lights; i++) {
        Light l = ubo.lights[i];
        float intensity = max(dot(normal_detailed, normalize(l.neg_direction)), 0.0);

        if(l.is_shadow_caster != 0.0) {
            intensity = min(intensity, shadow_values[shadow_values_index]);

            shadow_values_index++;
        }

        colour += intensity*l.light_intensity__and__shadow_pixel_offset.rgb;
    }

    if(SC_SSAO_ENABLED) {
        colour *= shadow_values.r*shadow_values.r;
    }

    /* Texture */

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


    const float fog = 1-smoothstep(0.00005, 0.001, gl_FragCoord.z);
    colour = mix(colour, vec3(4.5,5,5.5), fog); // blend to skybox colour (ish)

    colour = max(vec3(0), colour);

    out_colour = vec4(colour, alpha);
}