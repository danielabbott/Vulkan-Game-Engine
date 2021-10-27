#version 460

#include "common.glsl"


LOCATION(0) in vec3 pass_normal;
LOCATION(1) in vec2 pass_uv;
LOCATION(2) flat in int pass_draw_index;
LOCATION(3) in vec3 pass_position_model_space;
LOCATION(4) in mat3 pass_tangent_to_world;
LOCATION(7) in vec3 pass_position_world_space;

LOCATION(0) out vec4 out_colour;

BINDING(3) uniform sampler2D shadow_texture; // opengl binding 5

#if __VERSION__ >= 460

layout (constant_id = 0) const bool SC_SSAO_ENABLED = true;
layout (constant_id = 1) const bool SC_TRANSPARENT = false;

BINDING(4) uniform sampler2DShadow shadow_maps[4];
layout(binding = 5) uniform sampler2DArray textures[59];

#else

uniform sampler2DShadow shadow_map0; // opengl binding 0
uniform sampler2DShadow shadow_map1; // opengl binding 1
uniform sampler2DShadow shadow_map2; // opengl binding 2
uniform sampler2DShadow shadow_map3; // opengl binding 3
uniform sampler2DArray diffuse_texture; // opengl binding 4
uniform sampler2DArray nmap_texture; // opengl binding 6

#endif

#include "ubo.glsl"


void main() {
#if __VERSION__ >= 460
    #define data draw_objects.obj[pass_draw_index]
#else
    #define data draw_object.obj
#endif

    vec2 tex_coord = gl_FragCoord.xy / ubo.viewport_size;

    vec3 colour = ubo.ambient.rgb;
    vec4 shadow_values;
    
    if(!SC_TRANSPARENT) {
        shadow_values = texture(shadow_texture, tex_coord);
    }

    /* Normal map */

    vec3 normal = pass_normal;

    if(data.normal_map_sampler_index_plus1 >= 1.0) {
        vec2 t = texture(

#if __VERSION__ >= 460
            textures[data.normal_map_sampler_index_plus1-1], 
#else
            nmap_texture,
#endif

            vec3(pass_uv, data.normal_map_index)).rg * 2 - 1;
        
        normal = pass_tangent_to_world * vec3(t.x, t.y, sqrt(1 - t.x*t.x - t.y*t.y));
    }

    /* light */

    int shadow_values_index = SC_SSAO_ENABLED ? 1 : 0;
    for(int i = 0; i < ubo.number_of_lights; i++) {
        // Light l = ubo.lights[i];
        #define l ubo.lights[i]


        float intensity;

        if(l.world_pos_and_type.w == LIGHT_TYPE_DIRECTIONAL) {
            intensity = max(dot(normal, normalize(l.neg_direction_and_is_shadow_caster.xyz)), 0.0);
        }
        else {
            // point
            vec3 to_light = l.world_pos_and_type.xyz - pass_position_world_space;
            intensity = max(dot(normal, normalize(to_light)), 0.0);
            float dist_to_light = length(to_light);
            intensity /= max(0.01, dist_to_light*dist_to_light);
        }


        // TODO specular

        if(l.neg_direction_and_is_shadow_caster.w != 0.0) {
            if(SC_TRANSPARENT) {
                vec4 shadow_xyzw = data.modelViewProj[i+1] * vec4(pass_position_model_space, 1.0);
                shadow_xyzw.xy = shadow_xyzw.xy*0.5 + vec2(0.5);
                shadow_xyzw.z += 0.0001; // bias
                #ifndef VULKAN
                shadow_xyzw.y = 1-shadow_xyzw.y;
                #endif

#if __VERSION__ >= 460
                intensity *= texture(shadow_maps[i], vec3(shadow_xyzw.xy, shadow_xyzw.z));
#else
                if(i == 0)
                    intensity *= texture(shadow_map0, vec3(shadow_xyzw.xy, shadow_xyzw.z));
                else if (i == 1)
                    intensity *= texture(shadow_map1, vec3(shadow_xyzw.xy, shadow_xyzw.z));
                else if (i == 2)
                    intensity *= texture(shadow_map2, vec3(shadow_xyzw.xy, shadow_xyzw.z));
                else
                    intensity *= texture(shadow_map3, vec3(shadow_xyzw.xy, shadow_xyzw.z));
#endif
            }
            else {
                intensity = min(intensity, shadow_values[shadow_values_index]);
                shadow_values_index++;
            }
        }

        colour += intensity*l.light_intensity_and_shadow_pixel_offset.rgb;
        #undef l
    }

    if(SC_SSAO_ENABLED && !SC_TRANSPARENT) {
        colour *= shadow_values.r;
    }

    /* Texture */

    vec3 colour_without_light = data.colour.rgb;

    float alpha = 1;

    if(data.texture_sampler_index_plus1 >= 1.0) {
        vec4 tex_val = texture(
 #if __VERSION__ >= 460
            textures[data.texture_sampler_index_plus1-1], 
#else
            diffuse_texture,
#endif
            vec3(pass_uv, data.texture_index));


        tex_val.rgb *= 1.2;

        if(SC_TRANSPARENT) {
            colour_without_light *= tex_val.rgb;
            alpha = tex_val.a;
        }
        else {
            if(data.under_colour.a == 1) {
                colour_without_light *= mix(data.under_colour.rgb, tex_val.rgb, tex_val.a);
            }
            else {
                colour_without_light *= tex_val.rgb;
            }
        }
        
    }


    colour *= colour_without_light;
    colour += colour_without_light * data.colour.a; // luminosity


    float fog = 1-smoothstep(0.00005, 0.001, gl_FragCoord.z);
    colour = mix(colour, vec3(1,1,1), fog); // blend to skybox colour (ish)

    colour = max(vec3(0), colour);

    out_colour = vec4(colour, alpha);
}