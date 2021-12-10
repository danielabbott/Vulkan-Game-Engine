#version 450

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_position_world_space;

layout(location = 0) out vec4 out_colour;

layout(binding = 1) uniform sampler2D ssao_image;

layout(binding = 2) uniform sampler2DShadow shadow_maps[4];

#include "UBO.glsl"

layout(push_constant) uniform PushConstantsObject
{
	uint object_index;
} push_constants;

void main() {

    /* Ambient Occlusion */

    vec2 tex_coord = gl_FragCoord.xy / ubo.viewport_size;

    const float weights[4] = float[](
        0.1945945946,
        0.1216216216,
        0.0540540541,
        0.0162162162
    );

    const float centre_weight = 0.2270270270;

    // Centre value

	float occlusion = texture(ssao_image, tex_coord).r * centre_weight;
	
	// Others

	float x = 1.5;
	for(int i = 0; i < 4; i += 1) {
		occlusion += texture(ssao_image, vec2(tex_coord.x + x*ubo.one_pixel_x, tex_coord.y)).r
			* weights[i];
		occlusion += texture(ssao_image, vec2(tex_coord.x - x*ubo.one_pixel_x, tex_coord.y)).r
			* weights[i];
		x += 2;
	}

    ObjectUniformData data = ubo.objects[push_constants.object_index];
    occlusion *= 0.05 * data.ssao_intensity;



    /* Light */

    vec3 colour = max(vec3(0), ubo.ambient - vec3(occlusion));
    // vec3 colour = vec3(0);

    for(uint i = 0; i < ubo.number_of_lights; i++) {
        Light l = ubo.lights[i];
        float intensity = max(dot(in_normal, normalize(l.neg_direction)), 0.0);


        if(l.is_shadow_caster != 0.0 && intensity > 0.0) {
            vec4 shadow_xyzw = l.shadow_proj_view * vec4(in_position_world_space, 1.0);

            vec2 abs_xy = abs(shadow_xyzw.xy);
            if(abs_xy.x < 1.0 && abs_xy.y < 1.0 && shadow_xyzw.z < 1) {
                // shadow_xyzw.xyz /= shadow_xyzw.w; // Not needed for ortho matrix
                shadow_xyzw.xy = shadow_xyzw.xy*0.5 + vec2(0.5);

                // TODO better shadow blur

                const vec2 shadow_texture_offset = vec2(l.light_intensity__and__shadow_pixel_offset.w*2);

                // shadow_xyzw.z += 0.0001;

                intensity *= (
                    texture(shadow_maps[i], vec3(shadow_xyzw.xy, shadow_xyzw.z)).r +
                    texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(-1, -1), shadow_xyzw.z)).r +
                    texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(1, -1), shadow_xyzw.z)).r +
                    texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(-1, 1), shadow_xyzw.z)).r +
                    texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(1, 1), shadow_xyzw.z)).r
                ) / 5;    

                // intensity *= (
                //     // Centre
                //     4.0*texture(shadow_maps[i], vec3(shadow_xyzw.xy, shadow_xyzw.z)).r +

                //     // 2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(-0.5, -0.5), shadow_xyzw.z)).r +
                //     // 2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(0.5, -0.5), shadow_xyzw.z)).r +
                //     // 2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(-0.5, 0.5), shadow_xyzw.z)).r +
                //     // 2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(0.5, 0.5), shadow_xyzw.z)).r +

                //     // Outer corners
                //     texture(shadow_maps[i], vec3(shadow_xyzw.xy - 2.5*shadow_texture_offset, shadow_xyzw.z)).r +
                //     texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(2.5, -2.5), shadow_xyzw.z)).r +
                //     texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(-2.5, 2.5), shadow_xyzw.z)).r +
                //     texture(shadow_maps[i], vec3(shadow_xyzw.xy + 2.5*shadow_texture_offset, shadow_xyzw.z)).r + 

                //     // Top
                //     2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(-0.5, -2.5), shadow_xyzw.z)).r +
                //     2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(0.5, -2.5), shadow_xyzw.z)).r +

                //     // Bottom
                //     2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(-0.5, 2.5), shadow_xyzw.z)).r +
                //     2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(0.5, 2.5), shadow_xyzw.z)).r +

                //     // Left
                //     2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(-2.5, -0.5), shadow_xyzw.z)).r +
                //     2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(-2.5, 0.5), shadow_xyzw.z)).r +

                //     // Right
                //     2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(2.5, -0.5), shadow_xyzw.z)).r +
                //     2*texture(shadow_maps[i], vec3(shadow_xyzw.xy + shadow_texture_offset*vec2(2.5, 0.5), shadow_xyzw.z)).r
                // ) / 24;
            }
        }


        colour += intensity*l.light_intensity__and__shadow_pixel_offset.rgb;
    }

    colour *= data.colour.rgb;

    out_colour = vec4(max(vec3(0), colour), 1.0);


    

}