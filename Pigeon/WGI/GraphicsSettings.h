#pragma once

#include "Window.h"

namespace WGI {
	
	enum GPUPreference {
		Integrated,
		Dedicated
	};

	struct GraphicsSettings {
		WindowDimensions window_dimensions = WindowDimensions(1024,768);
		Window::FullscreenMode fullscreen_mode = Window::FullscreenMode::Windowed;
		GPUPreference gpu_preference = GPUPreference::Integrated;
	};
}