#pragma once

#include <stdint.h>
#include <pigeon/wgi/opengl/texture.h>

typedef struct PigeonOpenGLFramebuffer
{
    uint32_t id;
    unsigned int width, height;
} PigeonOpenGLFramebuffer;

PIGEON_ERR_RET pigeon_opengl_create_framebuffer(PigeonOpenGLFramebuffer*, 
    PigeonOpenGLTexture * depth, PigeonOpenGLTexture * colour);

void pigeon_opengl_bind_framebuffer(PigeonOpenGLFramebuffer*);

void pigeon_opengl_destroy_framebuffer(PigeonOpenGLFramebuffer*);
