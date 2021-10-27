#include <pigeon/wgi/opengl/texture.h>
#include <glad/glad.h>
#include <pigeon/wgi/image_formats.h>
#include <pigeon/wgi/opengl/limits.h>
#include <pigeon/assert.h>


static uint32_t bound_texture_id[8] = {0};
static unsigned int active_texture = 0;

static void pigeon_opengl_bind_texture_id(unsigned int binding, uint32_t new, GLenum target)
{
	assert(binding < 8);

	if(bound_texture_id[binding] != new) {
		if(active_texture != binding) {
			glActiveTexture(GL_TEXTURE0 + binding);
			active_texture = binding;
		}


		glBindTexture(target, new);
		bound_texture_id[binding] = new;
	}
}

GLenum pigeon_get_opengl_image_format(PigeonWGIImageFormat f)
{
	switch (f) {
		case PIGEON_WGI_IMAGE_FORMAT_NONE:
			return 0;



		case PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR:
			return GL_RGBA8;
		case PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB:
			return GL_SRGB8_ALPHA8;
		case PIGEON_WGI_IMAGE_FORMAT_BGRA_U8_SRGB:
			return GL_SRGB8_ALPHA8;
		case PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR:
			return GL_RGBA16F;
		case PIGEON_WGI_IMAGE_FORMAT_B10G11R11_UF_LINEAR:
			return GL_R11F_G11F_B10F;
		case PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32:
			return GL_DEPTH_COMPONENT32F;
		case PIGEON_WGI_IMAGE_FORMAT_DEPTH_U24:
			return GL_DEPTH_COMPONENT24;
		case PIGEON_WGI_IMAGE_FORMAT_DEPTH_U16:
			return GL_DEPTH_COMPONENT16;
		case PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR:
			return GL_R8;
		case PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR:
			return GL_RG8;
		case PIGEON_WGI_IMAGE_FORMAT_RG_U16_LINEAR:
			return GL_RG16;
		case PIGEON_WGI_IMAGE_FORMAT_A2B10G10R10_LINEAR:
			return GL_UNSIGNED_INT_2_10_10_10_REV;
		case PIGEON_WGI_IMAGE_FORMAT_BC1_SRGB:
			return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT; // GL_EXT_texture_compression_s3tc,EXT_texture_sRGB
		case PIGEON_WGI_IMAGE_FORMAT_BC3_SRGB:
			return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT; // GL_EXT_texture_compression_s3tc,EXT_texture_sRGB
		case PIGEON_WGI_IMAGE_FORMAT_BC5:
			return GL_COMPRESSED_RG_RGTC2; // GL_ARB_texture_compression_rgtc
		case PIGEON_WGI_IMAGE_FORMAT_BC7_SRGB:
			return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB; // GL_ARB_texture_compression_bptc
		case PIGEON_WGI_IMAGE_FORMAT_ETC1_LINEAR:
			 // Backwards compatible with OpenGLES 2
			return GL_COMPRESSED_SRGB8_ETC2; // GL_ARB_ES3_compatibility
		case PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB:
			return GL_COMPRESSED_SRGB8_ETC2; // GL_ARB_ES3_compatibility
		case PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB_ALPHA_SRGB:
			return GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC; // GL_ARB_ES3_compatibility
	}
	assert(false);
	return 0;
}

static void pigeon_get_opengl_image_format2(PigeonWGIImageFormat f, 
	GLenum* format_format, GLenum* format_type)
{
	switch (f) {
		case PIGEON_WGI_IMAGE_FORMAT_NONE:
			*format_format = *format_type = 0;
			break;



		case PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR:
			*format_format = GL_RGBA;
			*format_type = GL_UNSIGNED_BYTE;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB:
			*format_format = GL_RGBA;
			*format_type = GL_UNSIGNED_BYTE;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_BGRA_U8_SRGB:
			*format_format = GL_RGBA;
			*format_type = GL_UNSIGNED_BYTE;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR:
			*format_format = GL_RGBA;
			*format_type = GL_HALF_FLOAT;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_B10G11R11_UF_LINEAR:
			*format_format = GL_RGB;
			*format_type = GL_FLOAT;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32:
			*format_format = GL_DEPTH_COMPONENT;
			*format_type = GL_FLOAT;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_DEPTH_U24:
			*format_format = GL_DEPTH_COMPONENT;
			*format_type = GL_UNSIGNED_INT;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_DEPTH_U16:
			*format_format = GL_DEPTH_COMPONENT;
			*format_type = GL_UNSIGNED_SHORT;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR:
			*format_format = GL_RED;
			*format_type = GL_UNSIGNED_BYTE;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR:
			*format_format = GL_RG;
			*format_type = GL_UNSIGNED_BYTE;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_RG_U16_LINEAR:
			*format_format = GL_RG;
			*format_type = GL_UNSIGNED_SHORT;
			break;
		case PIGEON_WGI_IMAGE_FORMAT_A2B10G10R10_LINEAR:
			*format_format = GL_RGBA;
			*format_type = GL_UNSIGNED_INT_2_10_10_10_REV;
			break;
		
		case PIGEON_WGI_IMAGE_FORMAT_BC1_SRGB:
		case PIGEON_WGI_IMAGE_FORMAT_BC3_SRGB:
		case PIGEON_WGI_IMAGE_FORMAT_BC5:
		case PIGEON_WGI_IMAGE_FORMAT_BC7_SRGB:
		case PIGEON_WGI_IMAGE_FORMAT_ETC1_LINEAR:
		case PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB:
		case PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB_ALPHA_SRGB:
			// Compressed formats use glCompressedTexImage2D which does not take
			// the type parameter
			*format_format = pigeon_get_opengl_image_format(f);
			*format_type = 0;
			break;
	}
}


PIGEON_ERR_RET pigeon_opengl_create_texture(PigeonOpenGLTexture* texture, PigeonWGIImageFormat format,
	unsigned width, unsigned int height, unsigned int layers, unsigned int mip_levels)
{
    ASSERT_R1(texture && format && width && height && mip_levels);
	while(glGetError()) {}

	glGenTextures(1, &texture->id);
	ASSERT_R1(texture->id);

	GLenum target = layers > 0 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

	pigeon_opengl_bind_texture_id(0, texture->id, target);

	GLenum f = pigeon_get_opengl_image_format(format);

	if(GLAD_GL_ARB_texture_storage) {
		if(!layers) {
			glTexStorage2D(GL_TEXTURE_2D, mip_levels, f, width, height);
		}
		else {
			glTexStorage3D(GL_TEXTURE_2D_ARRAY, mip_levels, f, width, height, layers);	
		}
	}

	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, mip_levels > 1 ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mip_levels > 1 ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mip_levels-1);

	texture->format = format;
	texture->width = width;
	texture->height = height;
	texture->layers = layers;
	texture->mip_levels = mip_levels;
	
	if(glGetError()) {
        glDeleteTextures(1, &texture->id);
		texture->id = 0;
        ASSERT_R1(false);
    }
	return 0;
}

void pigeon_opengl_set_texture_sampler(PigeonOpenGLTexture * texture,
	bool linear_filter, bool anisiotropic_filtering,
	bool shadow_map, bool clamp_to_edge, bool multi_mip_map_sample)
{
	assert(texture && texture->id);

	GLenum target = texture->layers > 0 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
	pigeon_opengl_bind_texture_id(0, texture->id, target);

	if(shadow_map) linear_filter = true;

	if(texture->mip_levels > 1) {
		if(linear_filter) {
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, multi_mip_map_sample ?
			 	GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST);
		}
		else {
			glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}
	else {
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, linear_filter ? GL_LINEAR : GL_NEAREST);
	}

	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, linear_filter ? GL_LINEAR : GL_NEAREST);

	if(anisiotropic_filtering && GLAD_GL_EXT_texture_filter_anisotropic)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, pigeon_opengl_get_maximum_anisotropy());	

	if(shadow_map) {
		glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
	}

	if(clamp_to_edge) {
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else {
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

}


PIGEON_ERR_RET pigeon_opengl_upload_texture_mip(PigeonOpenGLTexture* texture,
	unsigned int mip, unsigned int layer, void * data, unsigned int data_size)
{
	ASSERT_R1(texture && texture->id && (!layer || layer < texture->layers) && mip < texture->mip_levels);
	ASSERT_R1(data);

	while(glGetError()) {}

	GLenum format_format, format_type; 
	pigeon_get_opengl_image_format2(texture->format, &format_format, &format_type);

	unsigned int w = texture->width, h = texture->height;
	for(unsigned int i = 0; i < mip; i++, w /= 2, h /= 2) {}
	if(!w) w = 1;
	if(!h) h = 1;

	ASSERT_R1(data_size >= (w*h* pigeon_image_format_bytes_per_4x4_block(texture->format)) / 16);

	if(texture->format >= PIGEON_WGI_IMAGE_FORMAT__FIRST_COMPRESSED_FORMAT &&
		texture->format <= PIGEON_WGI_IMAGE_FORMAT__LAST_COMPRESSED_FORMAT)
	{
		if(!texture->layers) {
			glCompressedTexSubImage2D(GL_TEXTURE_2D, mip, 0, 0, w, h, format_format, data_size, data);		
		}
		else {
			glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, mip, 0, 0, layer, w, h, 1, format_format, data_size, data);
		}
	}
	else {
		if(!texture->layers) {
			glTexSubImage2D(GL_TEXTURE_2D, mip, 0, 0, w, h, format_format, format_type, data);		
		}
		else {
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, mip, 0, 0, layer, w, h, 1, format_format, format_type, data);
		}
	}
    
    ASSERT_R1(!glGetError());
	return 0;
}

void pigeon_opengl_bind_texture(unsigned int binding, PigeonOpenGLTexture* texture)
{
	uint32_t new = 0;
	if(texture) {
		assert(texture->id);
		new = texture->id;
	}

	pigeon_opengl_bind_texture_id(binding, new, texture->layers ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D);
	
}


void pigeon_opengl_destroy_texture(PigeonOpenGLTexture* texture)
{
	assert(texture);
	if(texture && texture->id) {
		glDeleteTextures(1, &texture->id);
		texture->id = 0;
	}
}


PIGEON_ERR_RET pigeon_opengl_create_buffer_texture(PigeonOpenGLBufferTexture* texture,
	unsigned int elements, unsigned int components_per_element)
{
	assert(texture && elements && components_per_element && components_per_element <= 4);

	ASSERT_R1(!pigeon_opengl_create_buffer(&texture->buffer, 
		elements * components_per_element * 4, PIGEON_OPENGL_BUFFER_TYPE_TEXTURE));

	while(glGetError()) {}

	#define CLEANUP() pigeon_opengl_destroy_buffer_texture(texture);
	
	glGenTextures(1, &texture->id);
	ASSERT_R1(texture->id);
	pigeon_opengl_bind_texture_id(0, texture->id, GL_TEXTURE_BUFFER);

	GLenum f = 0;
	switch(components_per_element) {
		case 1:
			f = GL_R32F;
			break;
		case 2:
			f = GL_RG32F;
			break;
		case 3:
			f = GL_RGB32F;
			break;
		case 4:
			f = GL_RGBA32F;
			break;
	}

	glTexBuffer(GL_TEXTURE_BUFFER, f, texture->buffer.id);

	ASSERT_R1(!glGetError());
	#undef CLEANUP
	return 0;
}

void pigeon_opengl_bind_buffer_texture(unsigned int binding, PigeonOpenGLBufferTexture* texture)
{
	uint32_t new = 0;
	if(texture) {
		assert(texture->id);
		new = texture->id;
	}

	pigeon_opengl_bind_texture_id(binding, new, GL_TEXTURE_BUFFER);
	
}

void pigeon_opengl_destroy_buffer_texture(PigeonOpenGLBufferTexture* texture)
{
	assert(texture);
	
	if(texture) {
		pigeon_opengl_destroy_buffer(&texture->buffer);

		if(texture->id) {
			glDeleteTextures(1, &texture->id);
			texture->id = 0;
		}
	}

}


bool pigeon_opengl_bc1_optimal_available(void)
{
	return GLAD_GL_EXT_texture_compression_s3tc && GLAD_GL_EXT_texture_sRGB;
}

bool pigeon_opengl_bc3_optimal_available(void)
{
	return GLAD_GL_EXT_texture_compression_s3tc && GLAD_GL_EXT_texture_sRGB;
}

bool pigeon_opengl_bc5_optimal_available(void)
{
	return GLAD_GL_ARB_texture_compression_rgtc;
}

bool pigeon_opengl_bc7_optimal_available(void)
{
	return GLAD_GL_ARB_texture_compression_bptc;
}

bool pigeon_opengl_etc1_optimal_available(void)
{
	return GLAD_GL_ARB_ES3_compatibility;
}

bool pigeon_opengl_etc2_optimal_available(void)
{
	return GLAD_GL_ARB_ES3_compatibility;
}

bool pigeon_opengl_etc2_rgba_optimal_available(void)
{
	return GLAD_GL_ARB_ES3_compatibility;
}

