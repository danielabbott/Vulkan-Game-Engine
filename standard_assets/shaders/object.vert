#version 460

#include "common.glsl"

// OBJECT_DEPTH, OBJECT_DEPTH_ALPHA, OBJECT_LIGHT, OBJECT

/* inputs */

#if __VERSION__ >= 460
#define INPUT_POSITION_TYPE uvec4
#else
#define INPUT_POSITION_TYPE vec4
#endif

#ifdef SKINNED

#ifdef OBJECT_DEPTH

LOCATION(0) in INPUT_POSITION_TYPE in_position;
LOCATION(1) in uvec2 in_bone;

#elif defined(OBJECT_DEPTH_ALPHA)

LOCATION(0) in INPUT_POSITION_TYPE in_position;
LOCATION(1) in uvec2 in_bone;
LOCATION(2) in vec2 in_uv;

#elif defined(OBJECT_LIGHT)

LOCATION(0) in INPUT_POSITION_TYPE in_position;
LOCATION(1) in uvec2 in_bone;
LOCATION(2) in vec2 in_uv;
LOCATION(3) in vec4 in_normal;

#else // OBJECT

LOCATION(0) in INPUT_POSITION_TYPE in_position;
LOCATION(1) in uvec2 in_bone;
LOCATION(2) in vec2 in_uv;
LOCATION(3) in vec4 in_normal;
LOCATION(4) in vec4 in_tangent;

#define OBJECT

#endif


#define MATRIX_CONVERT(i, j, k) \
    mat4(i[0], i[1], i[2], 0.0, \
        i[3], j[0], j[1], 0.0, \
        j[2], j[3], k[0], 0.0, \
        k[1], k[2], k[3], 1.0)


#if __VERSION__ >= 460

layout(binding = 2, std140) readonly restrict buffer BonesObject {
    vec4 x[]; // 3 per bone
} bones;


vec4 get_bones_vec4(int first_bone_index, int bone_index, int i) {
    return bones.x[(first_bone_index + bone_index)*3 + i];
}

#else

layout(std140) uniform BonesUniform {
    vec4 x[768]; // 3 per bone
} bones;


vec4 get_bones_vec4(int first_bone_index, int bone_index, int i) {
    return bones.x[bone_index*3 + i];
}

#endif




#else

#ifdef OBJECT_DEPTH

LOCATION(0) in INPUT_POSITION_TYPE in_position;

#elif defined(OBJECT_DEPTH_ALPHA)

LOCATION(0) in INPUT_POSITION_TYPE in_position;
LOCATION(1) in vec2 in_uv;

#elif defined(OBJECT_LIGHT)

LOCATION(0) in INPUT_POSITION_TYPE in_position;
LOCATION(1) in vec2 in_uv;
LOCATION(2) in vec4 in_normal;

#else // OBJECT

LOCATION(0) in INPUT_POSITION_TYPE in_position;
LOCATION(1) in vec2 in_uv;
LOCATION(2) in vec4 in_normal;
LOCATION(3) in vec4 in_tangent;

#define OBJECT

#endif

#endif

/* outputs */

#ifdef OBJECT_DEPTH

#elif defined(OBJECT_DEPTH_ALPHA)

LOCATION(0) out vec2 pass_uv;
LOCATION(1) flat out int pass_draw_index;

#elif defined(OBJECT_LIGHT)

LOCATION(0) out vec3 pass_position_model_space;
LOCATION(1) out vec2 pass_uv;
LOCATION(2) flat out int pass_draw_index;
LOCATION(3) out vec3 pass_normal;

#else // OBJECT

LOCATION(0) out vec3 pass_normal;
LOCATION(1) out vec2 pass_uv;
LOCATION(2) flat out int pass_draw_index;
LOCATION(3) out vec3 pass_position_model_space;
LOCATION(4) out mat3 pass_tangent_to_world;

#endif

#include "ubo.glsl"



#if __VERSION__ >= 460

layout(push_constant) uniform PushConstantsObject
{
	int draw_index_offset;
    int model_view_proj_index;
} push_constants;

#define DRAW_INDEX_OFFSET push_constants.draw_index_offset
#define MODEL_VIEW_PROJ_INDEX push_constants.model_view_proj_index

#else

uniform int MODEL_VIEW_PROJ_INDEX;

#endif


void main() {

    
#if __VERSION__ >= 460
    float z = in_position.z / 1023.0;
    float x = ((in_position.x << 1) | (in_position.a & 1)) / 2047.0;
    float y = ((in_position.y << 1) | (in_position.a >> 1)) / 2047.0;
    const vec3 raw_position = vec3(x,y,z);
#else
    vec3 raw_position = in_position.xyz;
#endif

    
#if __VERSION__ >= 460
#ifdef VULKAN
    const int draw_index = DRAW_INDEX_OFFSET + gl_InstanceIndex;
#else
    const int draw_index = DRAW_INDEX_OFFSET + gl_InstanceID + gl_BaseInstance; // untested
#endif
    #define data draw_objects.obj[draw_index]
#else
    #define draw_index gl_InstanceID
    #define data draw_object.obj
#endif

    vec3 p = raw_position * data.position_range.xyz + data.position_min.xyz;

#if defined(OBJECT) || defined(OBJECT_LIGHT)
    mat3 nmat = mat3(data.normal_model_matrix);
#endif


    #ifdef SKINNED
    
#if __VERSION__ >= 460
    int first_bone_index = data.first_bone_index;
#else
    #define first_bone_index 0
#endif

    if(first_bone_index >= 0){
        int bone_index0 = int(in_bone.x >> 8u);
        int bone_index1 = int(in_bone.x & 0xffu);
        float bone_weight = float(in_bone.y) / 65535.0;

        // vec4 b_i = bones.x[(first_bone_index + bone_index0)*3];
        // vec4 b_j = bones.x[(first_bone_index + bone_index0)*3 + 1];
        // vec4 b_k = bones.x[(first_bone_index + bone_index0)*3 + 2];
        vec4 b_i = get_bones_vec4(first_bone_index, bone_index0, 0);
        vec4 b_j = get_bones_vec4(first_bone_index, bone_index0, 1);
        vec4 b_k = get_bones_vec4(first_bone_index, bone_index0, 2);

        mat4 m0 = MATRIX_CONVERT(b_i, b_j, b_k);

        // b_i = bones.x[(first_bone_index + bone_index1)*3];
        // b_j = bones.x[(first_bone_index + bone_index1)*3 + 1];
        // b_k = bones.x[(first_bone_index + bone_index1)*3 + 2];
        b_i = get_bones_vec4(first_bone_index, bone_index1, 0);
        b_j = get_bones_vec4(first_bone_index, bone_index1, 1);
        b_k = get_bones_vec4(first_bone_index, bone_index1, 2);

        mat4 m1 = MATRIX_CONVERT(b_i, b_j, b_k);

        vec3 p0 = (m0 * vec4(p, 1.0)).xyz;
        vec3 p1 = (m1 * vec4(p, 1.0)).xyz;
        p = mix(p1, p0, bone_weight);

        #if defined(OBJECT) || defined(OBJECT_LIGHT)
            vec3 n0 = (m0 * vec4(in_normal.xyz, 1.0)).xyz;
            vec3 n1 = (m1 * vec4(in_normal.xyz, 1.0)).xyz;
            pass_normal = normalize(nmat * mix(n1, n0, bone_weight));
        #endif
    }
    #endif
    
    
    gl_Position = data.modelViewProj[MODEL_VIEW_PROJ_INDEX] * vec4(p, 1.0);
    


    // UV, draw index
#ifndef OBJECT_DEPTH
    pass_uv = in_uv;
    pass_draw_index = draw_index;
#endif

    // position

#if defined(OBJECT_LIGHT) || defined(OBJECT)
    pass_position_model_space = p.xyz;
#endif

#if (defined(OBJECT) || defined(OBJECT_LIGHT)) && !defined(SKINNED)
// TODO multiply by scale reciprical instead of normalising
    pass_normal = normalize(nmat * in_normal.xyz);
#endif

    // tangent
#ifdef OBJECT
    vec3 normal_world_space = normalize(nmat * in_normal.xyz);
    vec3 tangent_world_space = normalize(nmat * in_tangent.xyz);
    pass_tangent_to_world = mat3(
        vec3(tangent_world_space), 
        vec3(cross(normal_world_space, tangent_world_space)*in_tangent.w), 
        vec3(normal_world_space) 
    );

#endif

}
