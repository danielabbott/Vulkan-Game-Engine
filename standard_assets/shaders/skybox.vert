#version 460

#include "common.glsl"

vec2 positions[3] = vec2[](
    vec2(-1.0, 5.0),
    vec2(5.0, -1.0),
    vec2(-1.0, -1.0)
);

LOCATION(0) out vec4 pass_direction;

#define NO_DRAW_OBJECTS
#include "ubo.glsl"


void main() {
    
#ifdef VULKAN
    vec2 p = positions[gl_VertexIndex];
    const vec2 uv = p;
#else
    vec2 p = positions[gl_VertexID];
    vec2 uv = p;
#endif

    mat4 v = ubo.view;
    v[3][0] = 0;
    v[3][1] = 0;
    v[3][2] = 0;
    pass_direction = inverse(ubo.proj * v) * vec4(uv, 1, 1);
    gl_Position = vec4(p, 0.0, 1.0);
}