#version 460

#include "common.glsl"

vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(5.0, -1.0),
    vec2(-1.0, 5.0)
);

LOCATION(0) out vec2 pass_tex_coord;

void main() {
    
#ifdef VULKAN
    vec2 p = positions[gl_VertexIndex];
    pass_tex_coord = p*0.5 + 0.5;
#else
    vec2 p = positions[gl_VertexID];
    pass_tex_coord = p*0.5 + 0.5;
    pass_tex_coord.y = 1.0-pass_tex_coord.y;
#endif


    gl_Position = vec4(p, 0.0, 1.0);
}