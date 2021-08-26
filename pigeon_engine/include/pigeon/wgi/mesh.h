#pragma once

#include "pipeline.h"
#include <pigeon/util.h>

struct PigeonVulkanStagedBuffer;
struct PigeonWGICommandBuffer;

typedef struct PigeonWGIMeshMeta
{

    PigeonWGIVertexAttributeType attribute_types[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES];

    // Relative to start of this meshes data
    uint64_t attribute_offsets[PIGEON_WGI_MAX_VERTEX_ATTRIBUTES];

    unsigned int vertex_count;
    unsigned int index_count;
    bool big_indices;

    float bounds_min[3];
    float bounds_range[3];

    // For when multiple meshes are stored together
    uint64_t start_offset;
} PigeonWGIMeshMeta;

uint64_t pigeon_wgi_mesh_meta_size_requirements(PigeonWGIMeshMeta*);

// identity or matrix for denormalising vertex position
void pigeon_wgi_get_mesh_matrix(PigeonWGIMeshMeta*, float*);

// Can contain multiple meshes
typedef struct PigeonWGIMultiMesh
{
    struct PigeonVulkanStagedBuffer * staged_buffer;
} PigeonWGIMultiMesh;

// mapping is set to a host-accessible write-only memory mapping
ERROR_RETURN_TYPE pigeon_wgi_create_mesh(PigeonWGIMultiMesh*, uint64_t size, void ** mapping);
ERROR_RETURN_TYPE pigeon_wgi_mesh_unmap(PigeonWGIMultiMesh*);
void pigeon_wgi_upload_mesh(struct PigeonWGICommandBuffer*, PigeonWGIMultiMesh*);
void pigeon_wgi_mesh_transfer_done(PigeonWGIMultiMesh*); // Call this when the transfer is done on the *GPU*
void pigeon_wgi_destroy_mesh(PigeonWGIMultiMesh*);
