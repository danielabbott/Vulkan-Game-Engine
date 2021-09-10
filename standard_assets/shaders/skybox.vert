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

#include "ubo.glsl"


void main() {
    vec2 p = positions[gl_VertexIndex];
    mat4 v = ubo.view;
    v[3][0] = 0;
    v[3][1] = 0;
    v[3][2] = 0;
    out_direction = inverse(ubo.proj * v) * vec4(p, 1, 1);
    gl_Position = vec4(p, 0.0, 1.0);
}