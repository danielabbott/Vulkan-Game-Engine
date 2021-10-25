#include <pigeon/wgi/opengl/buffer.h>
#include <glad/glad.h>
#include <pigeon/wgi/image_formats.h>
#include <pigeon/assert.h>
#include "gl.h"


static GLenum get_target(PigeonOpenGLBufferType type)
{
    switch(type) {
        case PIGEON_OPENGL_BUFFER_TYPE_VERTEX:
            return GL_ARRAY_BUFFER;
        case PIGEON_OPENGL_BUFFER_TYPE_INDEX:
            return GL_ELEMENT_ARRAY_BUFFER;
        case PIGEON_OPENGL_BUFFER_TYPE_UNIFORM:
            return GL_UNIFORM_BUFFER;
        case PIGEON_OPENGL_BUFFER_TYPE_TEXTURE:
            return GL_TEXTURE_BUFFER;
    }
    return 0;
}

PIGEON_ERR_RET pigeon_opengl_create_buffer(PigeonOpenGLBuffer* buffer, uint64_t size, PigeonOpenGLBufferType type)
{
    ASSERT_R1(buffer && size && size < 0x8000000000000000ull);
	while(glGetError()) {}

    glGenBuffers(1, &buffer->id);
    ASSERT_R1(buffer->id);

    GLenum t = get_target(type);

    glBindBuffer(t, buffer->id);

    glBufferData(t, (int64_t)size, NULL, 
        (type == PIGEON_OPENGL_BUFFER_TYPE_UNIFORM || type == PIGEON_OPENGL_BUFFER_TYPE_TEXTURE) 
            ? GL_STREAM_DRAW : GL_STATIC_DRAW);


    if(glGetError()) {
        glDeleteBuffers(1, &buffer->id);
        buffer->id = 0;
        ASSERT_R1(false);
    }

    buffer->type = type;
    buffer->size = size;

    return 0;
}

void * pigeon_opengl_map_buffer(PigeonOpenGLBuffer* buffer)
{
    ASSERT_R0(buffer && buffer->id && !buffer->mapping);


    buffer->mapping = glMapBufferRange(get_target(buffer->type), 0, buffer->size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    ASSERT_R0(buffer->mapping);
    return buffer->mapping;
}

void pigeon_opengl_unmap_buffer(PigeonOpenGLBuffer* buffer)
{
    assert(buffer && buffer->id);

    if(buffer->mapping) {
        glUnmapBuffer(get_target(buffer->type));
        buffer->mapping = NULL;
    }
}

void pigeon_opengl_bind_uniform_buffer2(PigeonOpenGLBuffer* buffer, unsigned int bind_point,
    unsigned int offset, unsigned int size)
{
    assert(buffer && buffer->id && buffer->type == PIGEON_OPENGL_BUFFER_TYPE_UNIFORM);
    assert(offset+size <= buffer->size);

    if(!size) size = (unsigned int) buffer->size - offset;
    assert(size);

    glBindBufferRange(GL_UNIFORM_BUFFER, bind_point, buffer->id, offset, size);
    
}

void pigeon_opengl_bind_uniform_buffer(PigeonOpenGLBuffer* buffer)
{
    assert(buffer && buffer->id && buffer->type == PIGEON_OPENGL_BUFFER_TYPE_UNIFORM);

    glBindBuffer(GL_UNIFORM_BUFFER, buffer->id);
}

void pigeon_opengl_bind_texture_buffer(PigeonOpenGLBuffer* buffer)
{
    assert(buffer && buffer->id && buffer->type == PIGEON_OPENGL_BUFFER_TYPE_TEXTURE);

    glBindBuffer(GL_TEXTURE_BUFFER, buffer->id);
}

void pigeon_opengl_destroy_buffer(PigeonOpenGLBuffer* buffer)
{
    assert(buffer);

    if(buffer && buffer->id) {
        glDeleteBuffers(1, &buffer->id);
        buffer->id = 0;
    }
}

static void get_vertex_attribute_format(PigeonWGIVertexAttributeType type, 
    unsigned int* comps, GLenum* gl_type, bool * norm, bool * gl_int)
{
	switch (type) {
		case PIGEON_WGI_VERTEX_ATTRIBUTE_NONE:
            assert(false);
			break;

		case PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION: 
		case PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR: 
            *comps = 3;
            *gl_type = GL_FLOAT;
            *norm = false;
            *gl_int = false;
            break;

		case PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION2D:
		case PIGEON_WGI_VERTEX_ATTRIBUTE_UV_FLOAT:
            *comps = 2;
            *gl_type = GL_FLOAT;
            *norm = false;
            *gl_int = false;
            break;
		case PIGEON_WGI_VERTEX_ATTRIBUTE_POSITION_NORMALISED: 
            *comps = 4;
            *gl_type = GL_UNSIGNED_INT_2_10_10_10_REV;
            *norm = true;
            *gl_int = false;
            break;

		case PIGEON_WGI_VERTEX_ATTRIBUTE_COLOUR_RGBA8: 
            *comps = 4;
            *gl_type = GL_UNSIGNED_BYTE;
            *norm = true;
            *gl_int = false;
            break;

		case PIGEON_WGI_VERTEX_ATTRIBUTE_NORMAL: 
		case PIGEON_WGI_VERTEX_ATTRIBUTE_TANGENT: 
            *comps = 4;
            *gl_type = GL_INT_2_10_10_10_REV;
            *norm = true;
            *gl_int = false;
            break;

		case PIGEON_WGI_VERTEX_ATTRIBUTE_UV: 
            *comps = 2;
            *gl_type = GL_UNSIGNED_SHORT;
            *norm = true;
            *gl_int = false;
            break;
		case PIGEON_WGI_VERTEX_ATTRIBUTE_BONE:
            *comps = 2;
            *gl_type = GL_UNSIGNED_SHORT;
            *norm = false;
            *gl_int = true;
            break;
	}
}


static uint32_t bound_vao = 0;

void pigeon_opengl_bind_vao_id(uint32_t new)
{
    if(new != bound_vao) {
        bound_vao = new;
        glBindVertexArray(new);
    }
}

PIGEON_ERR_RET pigeon_opengl_create_empty_vao(PigeonOpenGLVAO* vao)
{   
    ASSERT_R1(vao);

	while(glGetError()) {}


    glGenVertexArrays(1, &vao->id);
    ASSERT_R1(vao->id);


    pigeon_opengl_bind_vao_id(vao->id);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    if(glGetError()) {
        glDeleteVertexArrays(1, &vao->id);
        vao->id = 0;
        ASSERT_R1(false);
    }

    return 0;
}

PIGEON_ERR_RET pigeon_opengl_create_vao(PigeonOpenGLVAO* vao,
    unsigned int vertex_attribute_count, PigeonWGIVertexAttributeType vertex_attributes[],
    uint64_t attribute_start_offsets[], void ** vertex_data, uint64_t vertex_data_size,
    bool big_indices, unsigned int index_count, void ** index_data, uint64_t index_data_size)
{
    ASSERT_R1(vao && vertex_attribute_count && vertex_attributes);
    ASSERT_R1(attribute_start_offsets && vertex_data && vertex_data_size);
    ASSERT_R1(index_count && index_data && index_data_size);
    ASSERT_R1(index_data_size == (big_indices ? 4 : 2) * index_count);

	while(glGetError()) {}

    vao->big_indices = big_indices;

    glGenVertexArrays(1, &vao->id);
    ASSERT_R1(vao->id);

#define CLEANUP() pigeon_opengl_destroy_vao(vao);

    pigeon_opengl_bind_vao_id(vao->id);

    ASSERT_R1(!pigeon_opengl_create_buffer(&vao->vertex_buffer, 
        vertex_data_size, PIGEON_OPENGL_BUFFER_TYPE_VERTEX));
    
    *vertex_data = pigeon_opengl_map_buffer(&vao->vertex_buffer);
    ASSERT_R1(*vertex_data);
    

    ASSERT_R1(!pigeon_opengl_create_buffer(&vao->index_buffer, 
        index_data_size, PIGEON_OPENGL_BUFFER_TYPE_INDEX));

    *index_data = pigeon_opengl_map_buffer(&vao->index_buffer);
    ASSERT_R1(*index_data);


    for(unsigned int i = 0; i < vertex_attribute_count; i++) {
        unsigned int comps;
        GLenum gl_type;
        bool norm;
        bool gl_int = false;
        get_vertex_attribute_format(vertex_attributes[i], &comps, &gl_type, &norm, &gl_int);


        if(gl_int) glVertexAttribIPointer(i, comps, gl_type, 0, (void *)attribute_start_offsets[i]);
        else glVertexAttribPointer(i, comps, gl_type, norm, 0, (void *)attribute_start_offsets[i]);
        glEnableVertexAttribArray(i);
    }


    
    ASSERT_R1(!glGetError());
    
    
    return 0;
#undef CLEANUP
}


void pigeon_opengl_vao_unmap(PigeonOpenGLVAO* vao)
{
    assert(vao);

    pigeon_opengl_unmap_buffer(&vao->vertex_buffer);
    pigeon_opengl_unmap_buffer(&vao->index_buffer);
}

void pigeon_opengl_destroy_vao(PigeonOpenGLVAO* vao)
{
    assert(vao);

    if(vao && vao->id) {
        glDeleteVertexArrays(1, &vao->id);
        vao->id = 0;

        pigeon_opengl_destroy_buffer(&vao->vertex_buffer);
        pigeon_opengl_destroy_buffer(&vao->index_buffer);
    }
}