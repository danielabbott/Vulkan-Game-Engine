#pragma once

#include "Input.h"
#include <string>
#include <tuple>

struct GLFWwindow;

namespace WGI {

	enum class WindowVisibilityState {
		Minimised,
		Visible
	};

	struct WindowDimensions {
		unsigned int width;
		unsigned int height;

		WindowDimensions()
			: width(0), height(0) {}
		WindowDimensions(unsigned int w, unsigned int h)
			: width(w), height(h) {}
		WindowDimensions(unsigned int wh)
			: width(wh), height(wh) {}

		bool operator==(const WindowDimensions& b)
		{
			return width == b.width && height == b.height;
		}
		bool operator!=(const WindowDimensions& b)
		{
			return width != b.width || height != b.height;
		}

		bool invalid() const {
			return width == 0 or height == 0;
		}
	};

	using WindowTitle = std::string;

	class Window {
		GLFWwindow* window_handle;
		std::vector<KeyEvent> key_events;
		std::vector<MouseEvent> mouse_events;
		int scroll_amount = 0;
	public:
		enum class FullscreenMode {
			Windowed,
			Maximised,
			Fullscreen,
			ExclusiveFullscreen // lowest latency
		};

		// Dimensions ignored when fullscreen
		Window(WindowDimensions dimensions, WindowTitle const& title, FullscreenMode);
		void set_title(WindowTitle const& title);
		void set_dimensions(WindowDimensions dimensions);
		WindowDimensions get_dimensions() const;
		bool should_close(); // Close the window by destroying this object

		GLFWwindow* get_handle() const;



		bool key_is_down(Key);

		// Remember to clear the events after :)
		std::vector<KeyEvent> & get_key_events() { return key_events; }
		void clear_key_events() { key_events.clear(); }

		bool is_mouse_button_down(int button);
		std::vector<MouseEvent>& get_mouse_events(int& scroll) {
			scroll = scroll_amount;
			scroll_amount = 0;
			return mouse_events;
		}
		void clear_mouse_events() { mouse_events.clear(); }

		std::pair<int,int> get_mouse_position();
		std::tuple<int,int, float> get_mouse_position2();

		void grab_mouse(bool grabbed);



		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&&) = delete;
		Window& operator=(Window&&) = delete;


		~Window();

		void key_callback_(int key, int scancode, int action, int mods);
		void mouse_button_callback_(int button, int action, int mods);
		void scroll_callback_(int scroll);
	};

	void terminate_window_library();
}