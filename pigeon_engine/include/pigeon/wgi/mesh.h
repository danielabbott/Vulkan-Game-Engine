#pragma once

#include "pipeline.h"
#include <pigeon/util.h>

struct PigeonVulkanStagedBuffer;
struct PigeonWGICommandBuffer;

typedef struct PigeonWGIMeshMeta
{
    PigeonWGIVertexAttributeType attribute_types[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES];

    uint32_t vertex_count;
    uint32_t index_count;
    bool big_indices;

    float bounds_min[3];
    float bounds_range[3];

    uint32_t multimesh_start_vertex;
    uint32_t multimesh_start_index;
} PigeonWGIMeshMeta;

uint64_t pigeon_wgi_mesh_meta_size_requirements(PigeonWGIMeshMeta*);

// identity or matrix for denormalising vertex position
void pigeon_wgi_get_mesh_matrix(PigeonWGIMeshMeta*, float*);

// Multiple meshes (with same vertex attributes and index size) combined together
typedef struct PigeonWGIMultiMesh
{
    PigeonWGIVertexAttributeType attribute_types[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES];

    // Offsets into vulkan buffer
    uint64_t attribute_start_offsets[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES];
    uint64_t index_data_offset;

    uint32_t vertex_count;
    uint32_t index_count;
    bool big_indices;

    struct PigeonVulkanStagedBuffer * staged_buffer;
} PigeonWGIMultiMesh;

// mapping is set to a host-accessible write-only memory mapping
ERROR_RETURN_TYPE pigeon_wgi_create_multimesh(PigeonWGIMultiMesh*,
    PigeonWGIVertexAttributeType attribute_types[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES],
    unsigned int vertex_count, unsigned int index_count, bool big_indices, uint64_t* size, void ** mapping);
ERROR_RETURN_TYPE pigeon_wgi_multimesh_unmap(PigeonWGIMultiMesh*);
void pigeon_wgi_upload_multimesh(struct PigeonWGICommandBuffer*, PigeonWGIMultiMesh*);
void pigeon_wgi_multimesh_transfer_done(PigeonWGIMultiMesh*); // Call this when the transfer is done on the *GPU*
void pigeon_wgi_destroy_multimesh(PigeonWGIMultiMesh*);
