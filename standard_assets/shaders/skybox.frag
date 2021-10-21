#version 460

#include "common.glsl"

LOCATION(0) in vec4 pass_direction;

LOCATION(0) out vec4 out_colour;

void main() {    
    vec3 world_space_direction = normalize(pass_direction.xyz / pass_direction.w);

    float up_down = dot(world_space_direction, vec3(0,1,0)); // -1 to 1, down to up, full sphere

    const vec3 colours[3] = vec3[](
        vec3(5, 5, 5),
        vec3(0.5, 4, 10),
        vec3(0, 0.3, 10)
    );

    const float gradient_change_point = 0.2;

    const vec2 ranges[2] = vec2[] (
        vec2(0.0, gradient_change_point),
        vec2(gradient_change_point, 1.0)
    );

    // 0 or 1, colour gradiates from colours_index to colours_index+1
    int colours_index = int(ceil(max(0, up_down) - gradient_change_point));

    vec3 colour = mix(colours[colours_index], colours[colours_index+1],
        smoothstep(ranges[colours_index].x, ranges[colours_index].y, up_down));
    colour *= 0.3;

    out_colour = vec4(colour, 1);
}