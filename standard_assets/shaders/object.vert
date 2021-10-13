#version 460

// OBJECT_DEPTH, OBJECT_DEPTH_ALPHA, OBJECT_LIGHT, OBJECT

/* inputs */

#ifdef SKINNED

#ifdef OBJECT_DEPTH

layout(location = 0) in uvec4 in_position;
layout(location = 1) in uvec2 in_bone;

#elif defined(OBJECT_DEPTH_ALPHA)

layout(location = 0) in uvec4 in_position;
layout(location = 1) in uvec2 in_bone;
layout(location = 2) in vec2 in_uv;

#elif defined(OBJECT_LIGHT)

layout(location = 0) in uvec4 in_position;
layout(location = 1) in uvec2 in_bone;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_normal;

#else // OBJECT

layout(location = 0) in uvec4 in_position;
layout(location = 1) in uvec2 in_bone;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in vec4 in_normal;
layout(location = 4) in vec4 in_tangent;

#define OBJECT

#endif

struct Bone {
    float f[12];
};


#define MATRIX_CONVERT(f) \
    mat4(f[0], f[1], f[2], 0.0, \
        f[3], f[4], f[5], 0.0, \
        f[6], f[7], f[8], 0.0, \
        f[9], f[10], f[11], 1.0)

layout(binding = 2, std430) readonly restrict buffer BonesObject {
    Bone b[];
} bones;

#else

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

#endif

/* outputs */

#ifdef OBJECT_DEPTH

#elif defined(OBJECT_DEPTH_ALPHA)

layout(location = 0) out vec2 out_uv;
layout(location = 1) out flat uint out_draw_index;

#elif defined(OBJECT_LIGHT)

layout(location = 0) out vec3 out_position_model_space;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out flat uint out_draw_index;
layout(location = 3) out vec3 out_normal;

#else // OBJECT

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out flat uint out_draw_index;
layout(location = 3) out vec3 out_position_model_space;
layout(location = 4) out mat3 out_tangent_to_world;

#endif

#include "ubo.glsl"


layout(push_constant) uniform PushConstantsObject
{
	uint draw_index_offset;
    uint model_view_proj_index;
} push_constants;


void main() {
    float z = in_position.z / 1023.0;
    float x = ((in_position.x << 1) | (in_position.a & 1)) / 2047.0;
    float y = ((in_position.y << 1) | (in_position.a >> 1)) / 2047.0;

    uint draw_index = push_constants.draw_index_offset + gl_InstanceIndex;
    DrawObject data = draw_objects.obj[draw_index];
    vec3 p = vec3(x,y,z) * data.position_range.xyz + data.position_min.xyz;

    #if defined(OBJECT) || defined(OBJECT_LIGHT)
        mat3 nmat = mat3(data.normal_model_matrix);
    #endif

    #ifdef SKINNED
    if(data.first_bone_index != 0xffffffff){
        uint bone_index0 = in_bone.x >> 8;
        uint bone_index1 = in_bone.x & 0xff;
        float bone_weight = in_bone.y / 65535.0;

        Bone b0 = bones.b[data.first_bone_index + bone_index0];
        mat4 m0 = MATRIX_CONVERT(b0.f);
        Bone b1 = bones.b[data.first_bone_index + bone_index1];
        mat4 m1 = MATRIX_CONVERT(b1.f);

        vec3 p0 = (m0 * vec4(p, 1.0)).xyz;
        vec3 p1 = (m1 * vec4(p, 1.0)).xyz;
        p = mix(p1, p0, bone_weight);

        #if defined(OBJECT) || defined(OBJECT_LIGHT)
            vec3 n0 = (m0 * vec4(in_normal.xyz, 1.0)).xyz;
            vec3 n1 = (m1 * vec4(in_normal.xyz, 1.0)).xyz;
            out_normal = normalize(nmat * mix(n1, n0, bone_weight));
        #endif
    }
    #endif
    
    
    gl_Position = data.modelViewProj[push_constants.model_view_proj_index] * vec4(p, 1.0);
    


    // UV, draw index
#ifndef OBJECT_DEPTH
    out_uv = in_uv;
    out_draw_index = draw_index;
#endif

    // position

#if defined(OBJECT_LIGHT) || defined(OBJECT)
    out_position_model_space = p.xyz;
#endif

#if (defined(OBJECT) || defined(OBJECT_LIGHT)) && !defined(SKINNED)
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
