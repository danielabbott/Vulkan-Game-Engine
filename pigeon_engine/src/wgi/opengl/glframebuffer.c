#include <pigeon/wgi/opengl/texture.h>
#include <pigeon/wgi/opengl/framebuffer.h>
#include <glad/glad.h>
#include <pigeon/wgi/image_formats.h>
#include <pigeon/wgi/swapchain.h>
#include <pigeon/assert.h>

PIGEON_ERR_RET pigeon_opengl_create_framebuffer(PigeonOpenGLFramebuffer* fb, 
    PigeonOpenGLTexture * depth, PigeonOpenGLTexture * colour)
{
    ASSERT_R1(fb && (depth || colour));

    if(colour) {
        ASSERT_R1(colour->id);
    }
    if(depth) {
        ASSERT_R1(depth->id);

        if(colour) {
            ASSERT_R1(colour->width == depth->width && colour->height == depth->height);
        }
    }

	while(glGetError()) {}

    glGenFramebuffers(1, &fb->id);
	ASSERT_R1(fb->id);

    glBindFramebuffer(GL_FRAMEBUFFER, fb->id);

    if(colour) {
        if(!colour->layers) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colour->id, 0);
        }
        else {
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colour->id, 0, 0);
        }
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
    }
    else {
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    if(depth) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth->id, 0);
    }

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || glGetError()) {
        ERRLOG("Error creating OpenGL framebuffer");
        glDeleteFramebuffers(1, &fb->id);
        fb->id = 0;
        ASSERT_R1(false);
	}

    if(colour) {
        fb->width = colour->width;
        fb->height = colour->height;
    }
    else {
        fb->width = depth->width;
        fb->height = depth->height;
    }

	return 0;
}

void pigeon_opengl_bind_framebuffer(PigeonOpenGLFramebuffer* fb)
{
    if(fb) {
        assert(fb->id);

        glBindFramebuffer(GL_FRAMEBUFFER, fb->id);
        glViewport(0, 0, fb->width, fb->height);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        PigeonWGISwapchainInfo sc_info = pigeon_wgi_get_swapchain_info();
        glViewport(0, 0, sc_info.width, sc_info.height);
    }
}

void pigeon_opengl_destroy_framebuffer(PigeonOpenGLFramebuffer* fb)
{
    assert(fb);

    if(fb && fb->id) {
        glDeleteFramebuffers(1, &fb->id);
        fb->id = 0;
    }
}