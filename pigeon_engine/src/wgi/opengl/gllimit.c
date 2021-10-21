#include <glad/glad.h>
#include <pigeon/wgi/opengl/limits.h>

static int uniform_buffer_min_alignment;
static float max_aniso;

void pigeon_opengl_get_limits(void)
{
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_min_alignment);
    if(uniform_buffer_min_alignment < 1) uniform_buffer_min_alignment = 1;

    if(GLAD_GL_EXT_texture_filter_anisotropic)
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
}

unsigned int pigeon_opengl_get_uniform_buffer_min_alignment(void)
{
    return (unsigned) uniform_buffer_min_alignment;
}

float pigeon_opengl_get_maximum_anisotropy(void)
{
    return max_aniso;
}
