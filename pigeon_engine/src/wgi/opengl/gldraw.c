#include <pigeon/wgi/opengl/draw.h>
#include <pigeon/wgi/opengl/buffer.h>
#include <pigeon/wgi/pipeline.h>
#include <pigeon/assert.h>
#include <glad/glad.h>
#include "gl.h"

static PigeonWGIPipelineConfig old_config;

void pigeon_opengl_clear_depth()
{
    glDepthMask(true);
    old_config.depth_write = true;
    glClear(GL_DEPTH_BUFFER_BIT);
}


static bool first_set_state = true;

void pigeon_opengl_set_draw_state(PigeonWGIPipelineConfig const* config)
{
    if(first_set_state || config->cull_mode != old_config.cull_mode) {
        if(config->cull_mode == PIGEON_WGI_CULL_MODE_NONE) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
            glCullFace(config->cull_mode == PIGEON_WGI_CULL_MODE_FRONT ? GL_FRONT : GL_BACK);
        }
    }

    if(first_set_state || config->front_face != old_config.front_face) {
        glFrontFace(config->front_face == PIGEON_WGI_FRONT_FACE_ANTICLOCKWISE ?
            GL_CW : GL_CCW);
    }

    if(first_set_state || config->blend_function != old_config.blend_function) {
        if(config->blend_function == PIGEON_WGI_BLEND_NORMAL && !config->depth_only) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
        }
        else {
            glDisable(GL_BLEND);
        }
    }

    if(first_set_state || config->depth_test != old_config.depth_test) {
        if(config->depth_test) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
    }

    if(first_set_state || config->depth_write != old_config.depth_write) {
        if(config->depth_write) glDepthMask(true);
        else glDepthMask(false);
    }

    if(first_set_state || config->depth_cmp_equal != old_config.depth_cmp_equal) {
        if(config->depth_cmp_equal) glDepthFunc(GL_GEQUAL);
        else glDepthFunc(GL_GREATER);
    }

    if(first_set_state || config->depth_bias != old_config.depth_bias) {
        if(config->depth_bias) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(-1, 0);
        }
        else {
            glDisable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(0, 0);
        }
    }

    old_config = *config;
    first_set_state = false;
}

void pigeon_opengl_draw(PigeonOpenGLVAO* vao, unsigned int first, unsigned int count)
{
    assert(vao && vao->id);

    if(vao) {
        assert(vao->id);
        pigeon_opengl_bind_vao_id(vao->id);
    }
    glDrawArrays(GL_TRIANGLES, first, count);
}

void pigeon_opengl_draw_indexed(PigeonOpenGLVAO* vao, unsigned int start_vertex, unsigned int first, unsigned int count)
{
    assert(vao->id);

    pigeon_opengl_bind_vao_id(vao->id);
    
    glDrawElementsBaseVertex(GL_TRIANGLES, count, vao->big_indices ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
        (void *) (uintptr_t) (first * (vao->big_indices ? 4 : 2)), start_vertex);
}


