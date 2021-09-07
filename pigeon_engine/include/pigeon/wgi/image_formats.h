#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	PIGEON_WGI_IMAGE_FORMAT_NONE,

	// Colour formats

	PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_LINEAR,
	PIGEON_WGI_IMAGE_FORMAT_RGBA_U8_SRGB, // RGBA texture
	PIGEON_WGI_IMAGE_FORMAT_BGRA_U8_SRGB, // swapchain

	// HDR colour formats

	PIGEON_WGI_IMAGE_FORMAT_RGBA_F16_LINEAR,

	// Depth formats

	PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32,
	PIGEON_WGI_IMAGE_FORMAT_DEPTH_U24,

	// Linear non-colour formats
	PIGEON_WGI_IMAGE_FORMAT_R_U8_LINEAR,
	PIGEON_WGI_IMAGE_FORMAT_RG_U8_LINEAR,

	// Compressed formats (all srgb except ETC1). Not guaranteed to be supported

    PIGEON_WGI_IMAGE_FORMAT_BC1_SRGB, // RGB (dxt1, poor quality, GL_EXT_texture_compression_s3tc)
    PIGEON_WGI_IMAGE_FORMAT_BC3_SRGB, // RGBA (dxt3, poor quality, GL_EXT_texture_compression_s3tc)
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

static inline bool PIGEON_WGI_IMAGE_FORMAT_is_depth(PigeonWGIImageFormat format)
{
	return format == PIGEON_WGI_IMAGE_FORMAT_DEPTH_F32 || format == PIGEON_WGI_IMAGE_FORMAT_DEPTH_U24;
}

