#version 460

#include "common.glsl"


LOCATION(0) in vec3 pass_normal;
LOCATION(1) in vec2 pass_uv;
LOCATION(2) flat in int pass_draw_index;
LOCATION(3) in mat3 pass_tangent_to_world;
LOCATION(6) in vec3 pass_position_world_space;

LOCATION(0) out vec4 out_colour;

BINDING(3) uniform sampler2D ssao_texture; // opengl binding 5

#define SHADOW_TYPE_NOISY 0
#define SHADOW_TYPE_PCF4 1
#define SHADOW_TYPE_PCF16 2

#if __VERSION__ >= 460

layout (constant_id = 0) const bool SC_SSAO_ENABLED = true;
layout (constant_id = 1) const bool SC_TRANSPARENT = false;
layout (constant_id = 2) const int SC_SHADOW_TYPE = SHADOW_TYPE_PCF16;



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
#include "random.glsl"


#if __VERSION__ >= 460
    #define data draw_objects.obj[pass_draw_index]
#else
    #define data draw_object.obj
#endif

float sample_shadow_map(int i, vec3 coords_and_refz)
{
#if __VERSION__ >= 460
    return texture(shadow_maps[i], coords_and_refz);
#else
    if(i == 0)
        return texture(shadow_map0, coords_and_refz);
    else if (i == 1)
        return texture(shadow_map1, coords_and_refz);
    else if (i == 2)
        return texture(shadow_map2, coords_and_refz);
    else
        return texture(shadow_map3, coords_and_refz);
#endif

}

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 get_normal() {
    if(data.normal_map_sampler_index_plus1 >= 1.0) {
        vec2 t = texture(

#if __VERSION__ >= 460
            textures[data.normal_map_sampler_index_plus1-1], 
#else
            nmap_texture,
#endif

            vec3(pass_uv, data.normal_map_index)).rg * 2 - 1;
        
        vec2 tsqrd = t*t;
        return pass_tangent_to_world * vec3(t.x, t.y, sqrt(1 - tsqrd.x - tsqrd.y));
    }
    return pass_normal;
}

float calculate_light(int i, vec3 normal) {
#define light ubo.lights[i]

    float intensity;

    vec3 to_light_norm;

    if(light.world_pos_and_type.w == LIGHT_TYPE_DIRECTIONAL) {
        to_light_norm = normalize(light.neg_direction_and_is_shadow_caster.xyz);
        intensity = max(dot(normal, to_light_norm), 0.0);
    }
    else {
        // point
        vec3 to_light = light.world_pos_and_type.xyz - pass_position_world_space;
        float dist_to_light = length(to_light);
        to_light_norm = to_light / dist_to_light;
        intensity = max(dot(normal, to_light_norm), 0.0);
        intensity /= max(0.1, dist_to_light*dist_to_light);
    }

    if(data.specular_intensity > 0.0) {
        vec3 to_eye_norm = normalize(ubo.eye_position - pass_position_world_space);

        vec3 half_way_vector = normalize(to_light_norm + to_eye_norm);

        intensity *= 1+max(0.0, dot(half_way_vector, normal)-0.9) * data.specular_intensity;
    }


    return intensity;
#undef light
}

void calculate_basic_lighting(vec3 normal, out float intensities[4]) {
    for(int i = 0; i < ubo.number_of_lights && i < 4; i++) {
        intensities[i] = calculate_light(i, normal);
    }
}

vec3 apply_lighting(vec2 tex_coord, float intensities[4]) {
    vec3 colour = ubo.ambient.rgb;

	float random_value;

    if(SC_SHADOW_TYPE == SHADOW_TYPE_NOISY) {
        random_value = rand(tex_coord);
    }

    for(int i = 0; i < ubo.number_of_lights && i < 4; i++) {
        #define light ubo.lights[i]

        float intensity = intensities[i];

        if(light.neg_direction_and_is_shadow_caster.w != 0.0) {
            vec4 shadow_xyzw = light.shadow_proj_view * vec4(pass_position_world_space, 1.0);
            vec2 abs_xy = abs(shadow_xyzw.xy);
            shadow_xyzw.xy = shadow_xyzw.xy*0.5 + vec2(0.5);
            shadow_xyzw.z += 0.001; // bias
#ifndef VULKAN
            shadow_xyzw.y = 1.0 - shadow_xyzw.y;
#endif

            if(abs_xy.x < 1.0 && abs_xy.y < 1.0 && shadow_xyzw.z > 0) {
                float shadow = 0.0;
                #define shadow_texture_offset light.light_intensity_and_shadow_pixel_offset.w

                if(SC_SHADOW_TYPE == SHADOW_TYPE_NOISY) {
                    vec2 o = vec2(mod(random_value, 1.0), mod(random_value * 10.0, 1.0)) * 4 - vec2(2.0);
                    float d = shadow_xyzw.z;
                    if(o.x+o.y >= 2.0) d += 0.0005;
                    shadow = sample_shadow_map(i, vec3(shadow_xyzw.xy + o*shadow_texture_offset, d));
                }
                else if(SC_SHADOW_TYPE == SHADOW_TYPE_PCF4) {
                    for (float y = -0.5; y <= 0.5; y += 1.0) {   
                        for (float x = -0.5; x <= 0.5; x += 1.0) {
                            shadow += sample_shadow_map(i, vec3(shadow_xyzw.xy + vec2(x,y) * shadow_texture_offset, shadow_xyzw.z));
                        }
                    }
                    shadow /= 4.0; 
                }
                else { // SHADOW_TYPE_PCF16
                    for (float y = -1.5; y <= 1.5; y += 1.0) {   
                        for (float x = -1.5; x <= 1.5; x += 1.0) {
                            float d = shadow_xyzw.z;
                            if(abs(y) == 1.5 || abs(x) == 1.5) d += 0.0005;
                            shadow += sample_shadow_map(i, vec3(shadow_xyzw.xy + vec2(x,y) * shadow_texture_offset, d));
                        }
                    }
                    shadow /= 16.0; 
                }
                shadow *= shadow;
                intensity *= shadow;
                #undef shadow_texture_offset
            }
        }

        colour += intensity*light.light_intensity_and_shadow_pixel_offset.rgb;
        #undef l
    }
    return colour;
}

vec4 get_unlit_colour() {
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
    return vec4(colour_without_light, alpha);
}

void main() {
    vec2 tex_coord;
    if(SC_SSAO_ENABLED || SC_SHADOW_TYPE == SHADOW_TYPE_NOISY) {
        tex_coord = gl_FragCoord.xy * ubo.one_pixel;
    }


    vec3 normal = get_normal();

    float intensities[4];
    calculate_basic_lighting(normal, intensities);

    vec3 colour = apply_lighting(tex_coord, intensities);

    if(SC_SSAO_ENABLED) {
        colour *= vec3((1.0-texture(ssao_texture, tex_coord).r)*0.5+0.5 * data.ssao_intensity);
    }

    vec4 unlit_colour_and_alpha = get_unlit_colour();

    colour *= unlit_colour_and_alpha.rgb;
    colour += unlit_colour_and_alpha.rgb * data.colour.a; // luminosity


    // float fog = 1-smoothstep(0.00005, 0.001, gl_FragCoord.z);
    // colour = mix(colour, vec3(1,1,1), fog); // blend to skybox colour (ish)

    colour = max(vec3(0), colour);

    out_colour = vec4(colour, unlit_colour_and_alpha.a);
}