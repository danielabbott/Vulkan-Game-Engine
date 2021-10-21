#pragma once

#include <stdint.h>
#include <pigeon/util.h>
#include <pigeon/wgi/image_formats.h>
#include <pigeon/wgi/pipeline.h>

typedef enum {
    PIGEON_OPENGL_BUFFER_TYPE_UNIFORM,
    PIGEON_OPENGL_BUFFER_TYPE_VERTEX,
    PIGEON_OPENGL_BUFFER_TYPE_INDEX,
    PIGEON_OPENGL_BUFFER_TYPE_TEXTURE
} PigeonOpenGLBufferType;

typedef struct PigeonOpenGLBuffer
{
    uint64_t size;
    void * mapping;
    uint32_t id;
    PigeonOpenGLBufferType type;
} PigeonOpenGLBuffer;

PIGEON_ERR_RET pigeon_opengl_create_buffer(PigeonOpenGLBuffer*, uint64_t size, PigeonOpenGLBufferType type);


void pigeon_opengl_bind_uniform_buffer(PigeonOpenGLBuffer*);
void pigeon_opengl_bind_texture_buffer(PigeonOpenGLBuffer*);

void pigeon_opengl_bind_uniform_buffer2(PigeonOpenGLBuffer*, unsigned int bind_point,
    unsigned int offset, unsigned int size);

// Bind VAO or buffer first before calling these functions!
void * pigeon_opengl_map_buffer(PigeonOpenGLBuffer*);
void pigeon_opengl_unmap_buffer(PigeonOpenGLBuffer*);

void pigeon_opengl_destroy_buffer(PigeonOpenGLBuffer*);

typedef struct PigeonOpenGLVAO
{
    PigeonOpenGLBuffer vertex_buffer;
    PigeonOpenGLBuffer index_buffer;
    uint32_t id;
    uint32_t big_indices;
} PigeonOpenGLVAO;

PIGEON_ERR_RET pigeon_opengl_create_vao(PigeonOpenGLVAO*,
    unsigned int vertex_attribute_count, PigeonWGIVertexAttributeType vertex_attributes[],
    uint64_t attribute_start_offsets[], void ** vertex_data, uint64_t vertex_data_size,
    bool big_indices, unsigned int index_count, void ** index_data, uint64_t index_data_size);
PIGEON_ERR_RET pigeon_opengl_create_empty_vao(PigeonOpenGLVAO*); // no vertex data
void pigeon_opengl_vao_unmap(PigeonOpenGLVAO*);
void pigeon_opengl_destroy_vao(PigeonOpenGLVAO*);
