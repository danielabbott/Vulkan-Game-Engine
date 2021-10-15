#version 460


layout(location = 0) in vec2 in_uv;
layout(location = 1) in flat uint in_draw_index;


layout(binding = 5) uniform sampler2DArray textures[59];

#include "ubo.glsl"


void main() {
    DrawObject data = draw_objects.obj[in_draw_index];  

    float alpha = 1;

    // data.texture_sampler_index_plus1 >= 1
    vec2 uv = in_uv * data.texture_uv_base_and_range.zw + data.texture_uv_base_and_range.xy;
    float a = texture(textures[data.texture_sampler_index_plus1-1], vec3(uv, data.texture_index)).a;
    
    if(a < 1.0) {
        discard;
    }
}