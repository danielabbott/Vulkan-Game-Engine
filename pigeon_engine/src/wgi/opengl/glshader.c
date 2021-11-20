#include <config_parser.h>
#include <glad/glad.h>
#include <pigeon/assert.h>
#include <pigeon/misc.h>
#include <pigeon/wgi/opengl/shader.h>
#include <stdlib.h>
#include <string.h>

static uint32_t bound_shader = 0;

static void bind_shader_id(uint32_t new)
{
	if (new != bound_shader) {
		glUseProgram(new);
		bound_shader = new;
	}
}

static PIGEON_ERR_RET include_file(
	char** dst_, unsigned int* dst_sz_, unsigned* dst_i_, const char* directory_path, char* file_name, bool first_file)
{
	char* dst = *dst_;
	unsigned int dst_sz = *dst_sz_;
	unsigned int dst_i = *dst_i_;

	unsigned int j = 0;
	while (file_name[j] && file_name[j] != '"') {
		j++;
	}

	unsigned int dir_path_len = (unsigned)strlen(directory_path);
	char* full_path = malloc(dir_path_len + j + 1);
	ASSERT_R1(full_path);
	memcpy(full_path, directory_path, dir_path_len);
	memcpy(&full_path[dir_path_len], file_name, j);
	full_path[dir_path_len + j] = 0;

	unsigned long src_sz;
	char* src = (char*)pigeon_load_file(full_path, 1, &src_sz);
	free(full_path);
	ASSERT_R1(src);

	unsigned i = 0;
	if (first_file) {
		ASSERT_R1(string_starts_with(src, "#version 460") || string_starts_with(src, "#version 450"));
		i = 13;
	}

	unsigned int new_dst_sz = dst_sz + (unsigned int)src_sz - i;
	char* new_dst_ptr = realloc(dst, new_dst_sz);
	if (!new_dst_ptr) {
		free(src);
		ASSERT_R1(false);
	}

	dst = new_dst_ptr;
	dst_sz = new_dst_sz;

	for (; i < src_sz; i++) {
		if (string_starts_with(&src[i], "#include \"")) {
			i += 10;

			if (include_file(&dst, &dst_sz, &dst_i, directory_path, &src[i], false)) {
				free(src);
				return 1;
			}

			while (src[i] && src[i] != '"') {
				i++;
			}
		} else {
			dst[dst_i++] = src[i];
		}
	}

	*dst_ = dst;
	*dst_sz_ = dst_sz;
	*dst_i_ = dst_i;

	free(src);
	return 0;
}

PIGEON_ERR_RET pigeon_opengl_load_shader(PigeonOpenGLShader* shader, const char* file_path, const char* directory_path,
	const char* prefix, PigeonOpenGLShaderType type)
{
	ASSERT_R1(shader && file_path && directory_path);

	unsigned int prefix_len = prefix ? (unsigned)strlen(prefix) : 0;

	unsigned int dst_sz = 18 + prefix_len + 1 + 1;
	char* dst = malloc(dst_sz);
	ASSERT_R1(dst);

	memcpy(dst, "#version 330 core\n", 18);
	if (prefix)
		memcpy(&dst[18], prefix, prefix_len);
	dst[18 + prefix_len] = '\n';

	unsigned int dst_i = 18 + prefix_len + 1;
	if (include_file(&dst, &dst_sz, &dst_i, directory_path, (char*)file_path, true)) {
		free(dst);
		return 1;
	}

	dst[dst_sz - 1] = 0;
	dst[dst_i] = 0;

	while (glGetError()) { }

	shader->id = glCreateShader(type == PIGEON_OPENGL_SHADER_TYPE_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);

	if (!shader->id) {
		free(dst);
		ASSERT_R1(false);
	}

	glShaderSource(shader->id, 1, (const GLchar* const*)&dst, NULL);
	free(dst);

	glCompileShader(shader->id);

	GLint status;
	glGetShaderiv(shader->id, GL_COMPILE_STATUS, &status);

	if (!status) {
		GLint sz = 0;
		glGetShaderiv(shader->id, GL_INFO_LOG_LENGTH, &sz);

		if (sz > 0) {
			char* log = malloc((unsigned)sz + 1);

			if (log) {
				glGetShaderInfoLog(shader->id, sz, NULL, log);
				log[sz - 1] = 0;

				fprintf(stderr, "Shader compilation failure. Error log:\n%s\n", log);
				free(log);
			} else {
				ERRLOG("Shader compilation failure");
			}
		}

		glDeleteShader(shader->id);
		return 1;
	}

	if (glGetError()) {
		glDeleteShader(shader->id);
		shader->id = 0;
		ASSERT_R1(false);
	}
	return 0;
}

void pigeon_opengl_destroy_shader(PigeonOpenGLShader* shader)
{
	assert(shader);

	if (shader && shader->id) {
		glDeleteShader(shader->id);
	}
}

void pigeon_opengl_bind_shader_program(PigeonOpenGLShaderProgram* p)
{
	if (p) {
		assert(p->id);
		bind_shader_id(p->id);
	} else
		bind_shader_id(0);
}

PIGEON_ERR_RET pigeon_opengl_create_shader_program(PigeonOpenGLShaderProgram* program, PigeonOpenGLShader* vs,
	PigeonOpenGLShader* fs, unsigned int vertex_attrib_count, const char* vertex_attrib_names[])
{
	ASSERT_R1(program && vs && vs->id);
	if (fs)
		ASSERT_R1(fs->id);

	while (glGetError()) { }

	program->id = glCreateProgram();
	ASSERT_R1(program->id);

	glAttachShader(program->id, vs->id);
	if (fs)
		glAttachShader(program->id, fs->id);

	for (unsigned int i = 0; i < vertex_attrib_count; i++) {
		assert(vertex_attrib_names[i]);

		glBindAttribLocation(program->id, i, vertex_attrib_names[i]);
	}

	glLinkProgram(program->id);

	GLint status;
	glGetProgramiv(program->id, GL_LINK_STATUS, &status);

	if (!status) {
		GLint sz = 0;
		glGetProgramiv(program->id, GL_INFO_LOG_LENGTH, &sz);

		if (sz > 0) {
			char* log = malloc((unsigned)sz + 1);

			if (log) {
				glGetProgramInfoLog(program->id, sz, NULL, log);
				log[sz - 1] = 0;

				fprintf(stderr, "Shader program link failure. Error log: %s\n", log);
				free(log);
			} else {
				ERRLOG("Shader program link failure");
			}
		}

		glDeleteProgram(program->id);
	}

	if (glGetError()) {
		glDeleteProgram(program->id);
		program->id = 0;
		ASSERT_R1(false);
	}
	return 0;
}

void pigeon_opengl_set_uniform_buffer_binding(
	PigeonOpenGLShaderProgram* program, const char* name, unsigned int bind_point)
{
	assert(program && program->id && name);

	GLuint i = glGetUniformBlockIndex(program->id, name);

	if (i != GL_INVALID_INDEX) {
		glUniformBlockBinding(program->id, i, bind_point);
	} else
		assert(false);
}

void pigeon_opengl_set_shader_texture_binding_index(
	PigeonOpenGLShaderProgram* program, const char* name, unsigned int bind_point)
{
	assert(program && program->id && name);

	GLint l = glGetUniformLocation(program->id, name);

	if (l >= 0) {
		bind_shader_id(program->id);
		glUniform1i(l, bind_point);
	} else
		assert(false);
}

int pigeon_opengl_get_uniform_location(PigeonOpenGLShaderProgram* program, const char* name)
{
	assert(program && program->id && name);

	int l = glGetUniformLocation(program->id, name);
	assert(l >= 0);
	return l;
}

void pigeon_opengl_set_uniform_i(PigeonOpenGLShaderProgram* program, int location, int value)
{
	assert(program && program->id && location >= 0);

	bind_shader_id(program->id);
	glUniform1i(location, value);
}

void pigeon_opengl_set_uniform_f(PigeonOpenGLShaderProgram* program, int location, float value)
{
	assert(program && program->id && location >= 0);

	bind_shader_id(program->id);
	glUniform1f(location, value);
}

void pigeon_opengl_set_uniform_vec2(PigeonOpenGLShaderProgram* program, int location, float f0, float f1)
{
	assert(program && program->id && location >= 0);

	bind_shader_id(program->id);
	glUniform2f(location, f0, f1);
}

void pigeon_opengl_set_uniform_vec3(PigeonOpenGLShaderProgram* program, int location, float f0, float f1, float f2)
{
	assert(program && program->id && location >= 0);

	bind_shader_id(program->id);
	glUniform3f(location, f0, f1, f2);
}

void pigeon_opengl_set_uniform_vec4(
	PigeonOpenGLShaderProgram* program, int location, float f0, float f1, float f2, float f3)
{
	assert(program && program->id && location >= 0);

	bind_shader_id(program->id);
	glUniform4f(location, f0, f1, f2, f3);
}

void pigeon_opengl_destroy_shader_program(PigeonOpenGLShaderProgram* program)
{
	assert(program);

	if (program && program->id) {
		glDeleteProgram(program->id);
	}
}
