#version 450

// layout(location = 0) in vec3 in_world_space_direction;
layout(location = 0) in vec4 in_direction;

layout(location = 0) out vec4 out_colour;

#include "UBO.glsl"

// Not used.
layout(push_constant) uniform PushConstantsObject
{
	uint object_index;
} push_constants;

void main() {    
    vec3 world_space_direction = normalize(in_direction.xyz / in_direction.w);

    float up_down = dot(world_space_direction, vec3(0,1,0)); // This value is correct. -1 to 1, down to up

    float x = (clamp(up_down, -0.2, 0.2) + 0.2) * (1/0.4);
    // x = smoothstep(0, 1, x);
    x = pow(x, 2);
    
    // out_colour = vec4(vec3(up_down+1), 1.0);
    out_colour = vec4(mix(vec3(4, 5, 5), vec3(2, 7, 40), x), 1.0);

    // out_colour = vec4(vec3(up_down*0.5+0.5), 1.0);

    // if(up_down < -0.1) {
    //     out_colour=vec4(1,0,0,1);
    // }
    // if(up_down > 0.1) {
    //     out_colour=vec4(0,1,0,1);
    // }
    // out_colour=vec4(1,0,0,1);
}