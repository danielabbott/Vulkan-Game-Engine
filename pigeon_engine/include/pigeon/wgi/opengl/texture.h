#pragma once

#include <stdint.h>
#include <pigeon/util.h>
#include <pigeon/wgi/image_formats.h>
#include <pigeon/wgi/opengl/buffer.h>

typedef struct PigeonOpenGLTexture
{
    uint32_t id;
	PigeonWGIImageFormat format;
	unsigned int width, height, mip_levels;
	unsigned int layers; // 0 is 2D (1 layer), >= 1 is array texture
} PigeonOpenGLTexture;

#define PIGEON_OPENGL_2D_TEXTURE 0

PIGEON_ERR_RET pigeon_opengl_create_texture(PigeonOpenGLTexture*, PigeonWGIImageFormat,
	unsigned width, unsigned int height, unsigned int layers, unsigned int mip_levels);

void pigeon_opengl_set_texture_sampler(PigeonOpenGLTexture * texture,
	bool linear_filter, bool anisiotropic_filtering,
	bool shadow_map, bool clamp_to_edge, bool multi_mip_map_sample);

PIGEON_ERR_RET pigeon_opengl_upload_texture_mip(PigeonOpenGLTexture*,
	unsigned int mip, unsigned int layer, void * data, unsigned int data_size);

void pigeon_opengl_bind_texture(unsigned int binding, PigeonOpenGLTexture*);

void pigeon_opengl_destroy_texture(PigeonOpenGLTexture*);

unsigned int pigeon_get_opengl_image_format(PigeonWGIImageFormat f);

typedef struct PigeonOpenGLBufferTexture
{
	PigeonOpenGLBuffer buffer;
	uint32_t id;
} PigeonOpenGLBufferTexture;

PIGEON_ERR_RET pigeon_opengl_create_buffer_texture(PigeonOpenGLBufferTexture*,
	unsigned int elements, unsigned int components_per_element);
void pigeon_opengl_bind_buffer_texture(unsigned int binding, PigeonOpenGLBufferTexture*);
void pigeon_opengl_destroy_buffer_texture(PigeonOpenGLBufferTexture*);

bool pigeon_opengl_bc1_optimal_available(void);
bool pigeon_opengl_bc3_optimal_available(void);
bool pigeon_opengl_bc5_optimal_available(void);
bool pigeon_opengl_bc7_optimal_available(void);
bool pigeon_opengl_etc1_optimal_available(void);
bool pigeon_opengl_etc2_optimal_available(void);
bool pigeon_opengl_etc2_rgba_optimal_available(void);
