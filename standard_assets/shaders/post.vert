#version 460

#include "common.glsl"

vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(5.0, -1.0),
    vec2(-1.0, 5.0)
);

void main() {
    
#ifdef VULKAN
    vec2 p = positions[gl_VertexIndex];
#else
    vec2 p = positions[gl_VertexID];
#endif

    gl_Position = vec4(p, 0.0, 1.0);
}