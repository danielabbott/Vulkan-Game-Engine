#version 450

layout(location = 0) in uvec4 in_position;

#ifndef DEPTH_ONLY
layout(location = 1) in vec4 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_texture;


layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_position_world_space;
#endif

#include "UBO.glsl"

layout(push_constant) uniform PushConstantsObject
{
	uint object_index;
} push_constants;


void main() {
    float z = in_position.z / 1023.0;
    float x = ((in_position.x << 1) | (in_position.a & 1)) / 2047.0;
    float y = ((in_position.y << 1) | ((in_position.a & 2) >> 1)) / 2047.0;
    ObjectUniformData data = ubo.objects[push_constants.object_index];
    vec3 p = vec3(x,y,z) * data.position_range.xyz + data.position_min.xyz;
    
    gl_Position = data.modelViewProj * vec4(p, 1.0);
    

#ifndef DEPTH_ONLY
    // out_normal = normalize((data.model*in_normal).xyz);
    // TODO pass normal matrix in uniform buffer
    out_normal = normalize(( transpose(inverse(data.model)) * in_normal).xyz);
    out_position_world_space = (data.model * vec4(p, 1.0)).xyz;
#endif
}
