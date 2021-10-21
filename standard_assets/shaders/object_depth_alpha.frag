#version 460

#include "common.glsl"


LOCATION(0) in vec2 pass_uv;
LOCATION(1) flat in int pass_draw_index;



#if __VERSION__ >= 460
layout(binding = 5) uniform sampler2DArray textures[59];
#else
uniform sampler2DArray diffuse_texture; // opengl binding 4
uniform sampler2DArray nmap_texture; // opengl binding 7
#endif

#include "ubo.glsl"


void main() {
#if __VERSION__ >= 460
    #define data draw_objects.obj[pass_draw_index]
#else
    #define data draw_object.obj
#endif

    float alpha = 1;

    // data.texture_sampler_index_plus1 >= 1
    float a = texture(
#if __VERSION__ >= 460
            textures[data.texture_sampler_index_plus1-1], 
#else
            diffuse_texture,
#endif
        vec3(pass_uv, data.texture_index)).a;
    
    if(a < 1.0) {
        discard;
    }
}