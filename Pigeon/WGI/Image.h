#pragma once

namespace WGI {

	enum class ImageFormat {
		RGBALinearU8,
		SRGBU8,
		DepthF32, // If more depth formats are ever added, then a lot of code that compares to DepthF32 will need changing.
		RGBALinearF16,
		U8

		// change get_vulkan_image_format when adding new values
	};

}