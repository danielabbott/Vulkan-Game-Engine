#pragma once

typedef struct PigeonWGISwapchainInfo {
	unsigned int image_count; // always 3 for opengl
	PigeonWGIImageFormat format; // typically PIGEON_WGI_IMAGE_FORMAT_BGRA_U8_SRGB, meaningless for opengl
	unsigned int width, height;
} PigeonWGISwapchainInfo;

PigeonWGISwapchainInfo pigeon_wgi_get_swapchain_info(void);
