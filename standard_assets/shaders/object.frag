#version 460

layout (constant_id = 0) const bool SC_SSAO = true;
layout (constant_id = 1) const int SC_SHADOW_SAMPLES = 4;

layout(location = 0) in vec3 in_position_model_space;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in flat uint in_draw_call_index;
layout(location = 3) in mat3 in_tangent_to_world;

layout(location = 0) out vec4 out_colour;

layout(binding = 2) uniform sampler2D ssao_image;

layout(binding = 3) uniform sampler2D shadow_images[4];

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
        intensity *= texture(shadow_images[i], tex_coord).r;

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