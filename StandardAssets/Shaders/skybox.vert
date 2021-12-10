#version 450

// vec2 positions[3] = vec2[](
//     vec2(-1.0, -1.0),
//     vec2(5.0, -1.0),
//     vec2(-1.0, 5.0)
// );

vec2 positions[3] = vec2[](
    vec2(-1.0, 5.0),
    vec2(5.0, -1.0),
    vec2(-1.0, -1.0)
);

// layout(location = 0) out vec3 out_world_space_direction;
layout(location = 0) out vec4 out_direction;

#include "UBO.glsl"

layout(push_constant) uniform PushConstantsObject
{
	vec2 viewport_dimensions;
	vec2 one_pixel; // 1.0 / viewport_dimensions
} push_constants;


void main() {
    vec2 p = positions[gl_VertexIndex];
    out_direction = inverse(ubo.proj * ubo.view) * vec4(p, 1, 1);
    gl_Position = vec4(p, 1.0, 1.0);
}