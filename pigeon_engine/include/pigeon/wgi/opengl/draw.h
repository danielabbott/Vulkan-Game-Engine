#pragma once

struct PigeonOpenGLVAO;
struct PigeonWGIPipelineConfig;

void pigeon_opengl_clear_depth(void);

void pigeon_opengl_set_draw_state(struct PigeonWGIPipelineConfig const* config);

// Bind (or unbind) VAO first

void pigeon_opengl_draw(struct PigeonOpenGLVAO*, unsigned int first, unsigned int count);
void pigeon_opengl_draw_indexed(struct PigeonOpenGLVAO*, unsigned int start_vertex, unsigned int first, unsigned int count);
