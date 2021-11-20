#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	PIGEON_WGI_IMAGE_FORMAT_NONE,

	// SRGB Colour formats

	PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB, // RGBA texture
	PIGEON_WGI_IMAGE_FORMAT_BGRA_U8_SRGB, // swapchain

	// HDR linear colour formats

	PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR,
	PIGEON_WGI_IMAGE_FORMAT_B10G11R11_UF_LINEAR, // Support not guaranteed

	// Depth formats

	PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32,
	PIGEON_WGI_IMAGE_FORMAT_DEPTH_U24,
	PIGEON_WGI_IMAGE_FORMAT_DEPTH_U16,

	// Linear colour/data formats

	PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR,
	PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR,
	PIGEON_WGI_IMAGE_FORMAT_RG_U16_LINEAR,
	PIGEON_WGI_IMAGE_FORMAT_A2B10G10R10_LINEAR,
	PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR,

	// Compressed formats (all srgb except ETC1). Not guaranteed to be supported

	PIGEON_WGI_IMAGE_FORMAT_BC1_SRGB, // RGB (dxt1, poor quality, GL_EXT_texture_compression_s3tc+EXT_texture_sRGB)
	PIGEON_WGI_IMAGE_FORMAT_BC3_SRGB, // RGBA (dxt3, poor quality, GL_EXT_texture_compression_s3tc+EXT_texture_sRGB)
	PIGEON_WGI_IMAGE_FORMAT_BC5, // RG (ARB_texture_compression_rgtc)
	PIGEON_WGI_IMAGE_FORMAT_BC7_SRGB, // RGB(A) (good quality, ARB_texture_compression_bptc)

	PIGEON_WGI_IMAGE_FORMAT_ETC1_LINEAR, // RGB (mobile, poor quality, GL_ETC1_RGB8_OES)
	PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB, // RGB (mobile, good quality, GL_COMPRESSED_SRGB8_ETC2)
	PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB_ALPHA_SRGB, // RGBA (mobile, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC)

	PIGEON_WGI_IMAGE_FORMAT__FIRST_COMPRESSED_FORMAT = PIGEON_WGI_IMAGE_FORMAT_BC1_SRGB,
	PIGEON_WGI_IMAGE_FORMAT__LAST_COMPRESSED_FORMAT = PIGEON_WGI_IMAGE_FORMAT_ETC2_SRGB_ALPHA_SRGB,

	// change pigeon_get_vulkan_image_format when adding new values
} PigeonWGIImageFormat;

unsigned int pigeon_image_format_bytes_per_pixel(PigeonWGIImageFormat);
unsigned int pigeon_image_format_bytes_per_4x4_block(PigeonWGIImageFormat);

static inline bool pigeon_wgi_image_format_is_depth(PigeonWGIImageFormat format)
{
	return format == PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32 || format == PIGEON_WGI_IMAGE_FORMAT_DEPTH_U24
		|| format == PIGEON_WGI_IMAGE_FORMAT_DEPTH_U16;
}

bool pigeon_wgi_bc1_optimal_available(void);
bool pigeon_wgi_bc3_optimal_available(void);
bool pigeon_wgi_bc5_optimal_available(void);
bool pigeon_wgi_bc7_optimal_available(void);
bool pigeon_wgi_etc1_optimal_available(void);
bool pigeon_wgi_etc2_optimal_available(void);
bool pigeon_wgi_etc2_rgba_optimal_available(void);
