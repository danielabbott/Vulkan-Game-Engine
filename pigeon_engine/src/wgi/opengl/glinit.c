#include <glad/glad.h>
#include <pigeon/assert.h>
#include <pigeon/util.h>

#ifdef DEBUG
static void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar * message, const void * arg0)
{
	(void) source;
	(void) id;
	(void) severity;
	(void) length;
	(void) arg0;
    if (type != GL_DEBUG_TYPE_OTHER_ARB) {
        fprintf(stderr, "OpenGL error: %s\n", message);

		assert(type != GL_DEBUG_TYPE_ERROR_ARB);
    }
}
#endif

PIGEON_ERR_RET pigeon_opengl_init(void);
PIGEON_ERR_RET pigeon_opengl_init(void)
{
    if(!GL_ARB_clip_control) {
        ERRLOG("GL_ARB_clip_control not supported");
        return 1;
    }

    glClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
    glClearDepth(0.0);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_CLAMP);

    printf("Device: %s\n", glGetString(GL_RENDERER));

    #ifdef DEBUG
    if (GL_ARB_debug_output != 0) {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageCallbackARB(debug_callback, NULL);
    }
    #endif
    return 0;
}
