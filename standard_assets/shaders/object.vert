#version 460

// OBJECT_DEPTH, OBJECT_DEPTH_ALPHA, OBJECT_LIGHT, OBJECT

/* inputs */

#ifdef OBJECT_DEPTH

layout(location = 0) in uvec4 in_position;

#elif defined(OBJECT_DEPTH_ALPHA)

layout(location = 0) in uvec4 in_position;
layout(location = 1) in vec2 in_uv;

#elif defined(OBJECT_LIGHT)

layout(location = 0) in uvec4 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_normal;

#else // OBJECT

layout(location = 0) in uvec4 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_normal;
layout(location = 3) in vec4 in_tangent;

#define OBJECT

#endif

/* outputs */

#ifdef OBJECT_DEPTH

#elif defined(OBJECT_DEPTH_ALPHA)

layout(location = 0) out vec2 out_uv;
layout(location = 1) out flat uint out_draw_call_index;

#elif defined(OBJECT_LIGHT)

layout(location = 0) out vec3 out_position_model_space;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out flat uint out_draw_call_index;
layout(location = 3) out vec3 out_normal;

#else // OBJECT

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out flat uint out_draw_call_index;
layout(location = 3) out mat3 out_tangent_to_world;

#endif


#include "ubo.glsl"

layout(push_constant) uniform PushConstantsObject
{
	uint draw_call_index_offset;
    uint model_view_proj_index;
} push_constants;


void main() {
    float z = in_position.z / 1023.0;
    float x = ((in_position.x << 1) | (in_position.a & 1)) / 2047.0;
    float y = ((in_position.y << 1) | (in_position.a >> 1)) / 2047.0;

    uint draw_call_index = push_constants.draw_call_index_offset + gl_DrawID + gl_InstanceIndex;
    DrawCallObject data = draw_call_objects.obj[draw_call_index];
    vec3 p = vec3(x,y,z) * data.position_range.xyz + data.position_min.xyz;
    
    gl_Position = data.modelViewProj[push_constants.model_view_proj_index] * vec4(p, 1.0);


    // UV, draw call index
#ifndef OBJECT_DEPTH
    out_uv = in_uv;
    out_draw_call_index = draw_call_index;
#endif

    // position

#ifdef OBJECT_LIGHT
    out_position_model_space = p.xyz;
#endif

#if defined(OBJECT) || defined(OBJECT_LIGHT)
    mat3 nmat = mat3(data.normal_model_matrix);
    out_normal = normalize(nmat * in_normal.xyz);
#endif

    // tangent
#ifdef OBJECT
    vec3 normal_world_space = normalize(nmat * in_normal.xyz);
    vec3 tangent_world_space = normalize(nmat * in_tangent.xyz);
    out_tangent_to_world = mat3(
        vec3(tangent_world_space), 
        vec3(cross(normal_world_space, tangent_world_space)*in_tangent.w), 
        vec3(normal_world_space) 
    );
#endif

}
