#pragma once

#include <pigeon/util.h>
#include <stdint.h>

typedef struct PigeonOpenGLShader {
	uint32_t id;
} PigeonOpenGLShader;

typedef enum { PIGEON_OPENGL_SHADER_TYPE_VERTEX, PIGEON_OPENGL_SHADER_TYPE_FRAGMENT } PigeonOpenGLShaderType;

PIGEON_ERR_RET pigeon_opengl_load_shader(
	PigeonOpenGLShader*, const char* file_path, const char* directory_path, const char* prefix, PigeonOpenGLShaderType);

// GL4.6
// PIGEON_ERR_RET pigeon_opengl_create_shader(PigeonOpenGLShader*, const uint32_t* spv, unsigned int spv_size);

void pigeon_opengl_destroy_shader(PigeonOpenGLShader*);

typedef struct PigeonOpenGLShaderProgram {
	uint32_t id;
} PigeonOpenGLShaderProgram;

// N.b. Shader objects can be deleted after creating a shader program
// prefix is a string added just after the #version
PIGEON_ERR_RET pigeon_opengl_create_shader_program(PigeonOpenGLShaderProgram*, PigeonOpenGLShader* vs,
	PigeonOpenGLShader* fs, unsigned int vertex_attrib_count, const char* vertex_attrib_names[]);

void pigeon_opengl_bind_shader_program(PigeonOpenGLShaderProgram*);

void pigeon_opengl_set_uniform_buffer_binding(PigeonOpenGLShaderProgram*, const char* name, unsigned int bind_point);

void pigeon_opengl_set_shader_texture_binding_index(
	PigeonOpenGLShaderProgram*, const char* name, unsigned int bind_point);

int pigeon_opengl_get_uniform_location(PigeonOpenGLShaderProgram*, const char* name);
void pigeon_opengl_set_uniform_i(PigeonOpenGLShaderProgram*, int location, int value);
void pigeon_opengl_set_uniform_f(PigeonOpenGLShaderProgram*, int location, float value);

void pigeon_opengl_set_uniform_vec2(PigeonOpenGLShaderProgram*, int location, float f0, float f1);
void pigeon_opengl_set_uniform_vec3(PigeonOpenGLShaderProgram*, int location, float f0, float f1, float f2);
void pigeon_opengl_set_uniform_vec4(PigeonOpenGLShaderProgram*, int location, float f0, float f1, float f2, float f3);

void pigeon_opengl_destroy_shader_program(PigeonOpenGLShaderProgram*);
