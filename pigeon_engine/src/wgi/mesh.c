#include <pigeon/wgi/mesh.h>
#include <pigeon/util.h>
#include <pigeon/wgi/vulkan/buffer.h>
#include <stddef.h>
#include "singleton.h"

uint64_t pigeon_wgi_mesh_meta_size_requirements(PigeonWGIMeshMeta *meta)
{
    assert(meta);

    uint64_t sz = 0;

    for (unsigned int i = 0; i < PIGEON_WGI_MAX_VERTEX_ATTRIBUTES; i++)
    {
        PigeonWGIVertexAttributeType type = meta->attribute_types[i];
        if (!type)
            break;

        sz += pigeon_wgi_get_vertex_attribute_type_size(type);
    }

    unsigned int index_size = meta->big_indices ? 4 : 2;

    return sz*(uint64_t)meta->vertex_count + (uint64_t)(index_size*meta->index_count);
}

ERROR_RETURN_TYPE pigeon_wgi_create_mesh(PigeonWGIMultiMesh *mesh, uint64_t size, void **mapping)
{
    ASSERT_1(mesh);

    mesh->staged_buffer = calloc(1, sizeof *mesh->staged_buffer);
    ASSERT_1(mesh->staged_buffer);

#define OOPS()                      \
    {                               \
        free(mesh->staged_buffer);  \
        mesh->staged_buffer = NULL; \
        return 1;                   \
    }

    PigeonVulkanBufferUsages usages = {0};
    usages.vertices = usages.indices = true;
    if (pigeon_vulkan_create_staged_buffer(mesh->staged_buffer, size, usages)) {
        OOPS();
    }

    if (pigeon_vulkan_staged_buffer_map(mesh->staged_buffer, mapping)) {
        OOPS();
    }

#undef OOPS

    return 0;
}

ERROR_RETURN_TYPE pigeon_wgi_mesh_unmap(PigeonWGIMultiMesh *mesh)
{
    assert(mesh && mesh->staged_buffer);

    ASSERT_1(!pigeon_vulkan_staged_buffer_write_done(mesh->staged_buffer));
    return 0;
}


void pigeon_wgi_upload_mesh(PigeonWGICommandBuffer* cmd, PigeonWGIMultiMesh* mesh)
{
    assert(mesh && mesh->staged_buffer);

    if(!mesh->staged_buffer->buffer_is_host_visible) {
        pigeon_vulkan_staged_buffer_transfer(mesh->staged_buffer, &cmd->command_pool, 0);
    }
}

void pigeon_wgi_mesh_transfer_done(PigeonWGIMultiMesh* mesh)
{
    assert(mesh && mesh->staged_buffer);

    if(!mesh->staged_buffer->buffer_is_host_visible) {
        pigeon_vulkan_staged_buffer_transfer_complete(mesh->staged_buffer);
    }
}

void pigeon_wgi_destroy_mesh(PigeonWGIMultiMesh *mesh)
{
    if (mesh && mesh->staged_buffer)
    {
        pigeon_vulkan_destroy_staged_buffer(mesh->staged_buffer);
        free(mesh->staged_buffer);
        mesh->staged_buffer = NULL;
    }
    else
    {
        assert(false);
    }
}
