#include "Window.h"
#include "../deps/magic_enum.hpp"

#define GLFW_INCLUDE_NONE
#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>


#include <stdexcept>

using namespace std;

namespace WGI {

	void init_tablet(GLFWwindow*);
	void deinit_tablet();

	static bool glfw_init_done = false;

	static struct GLFWTerminator {
		~GLFWTerminator() {
			if(glfw_init_done) {
				deinit_tablet();
				glfwTerminate();
				glfw_init_done = false;
			}
		}
	} __glfw_terminator;

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void scroll_callback(GLFWwindow* window, double x, double y);

	Window::Window(WindowDimensions dimensions, WindowTitle const& title, FullscreenMode fullscreen)
	{
		if (!glfw_init_done) {
			if (glfwInit() != GLFW_TRUE) {
				throw runtime_error("glfwInit error");
			}
			if (glfwVulkanSupported() != GLFW_TRUE) {
				glfwTerminate();
				throw runtime_error("Vulkan not supported");
			}
			glfw_init_done = true;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		GLFWwindow* window = nullptr;
		if (fullscreen == FullscreenMode::Windowed || fullscreen == FullscreenMode::Maximised) {
			window = glfwCreateWindow(dimensions.width, dimensions.height, title.c_str(), nullptr, nullptr);
		}
		else if (fullscreen == FullscreenMode::ExclusiveFullscreen) {	
			auto monitor = glfwGetPrimaryMonitor();
			auto& mode = *glfwGetVideoMode(monitor);		
			glfwWindowHint(GLFW_RED_BITS, mode.redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode.greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode.blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode.refreshRate);
			window = glfwCreateWindow(mode.width, mode.height, title.c_str(), monitor, nullptr);
		}
		else if (fullscreen == FullscreenMode::Fullscreen) {
			auto monitor = glfwGetPrimaryMonitor();
			auto& mode = *glfwGetVideoMode(monitor);
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
			window = glfwCreateWindow(mode.width, mode.height, title.c_str(), nullptr, nullptr);
		}
		
		if (fullscreen == FullscreenMode::Maximised) {
			glfwMaximizeWindow(window);
		}

		if (!window) {
			throw runtime_error("glfwCreateWindow error");
		}

		window_handle = window;

		init_tablet(window_handle);

		glfwSetWindowUserPointer(window, this);
		glfwSetKeyCallback(window, key_callback);
		glfwSetMouseButtonCallback(window, mouse_button_callback);
		glfwSetScrollCallback(window, scroll_callback);
	}
	
	void Window::grab_mouse(bool grabbed)
	{
		glfwSetInputMode(window_handle, GLFW_CURSOR, grabbed ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);	
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Window& w = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		w.key_callback_(key, scancode, action, mods);
	}

	void Window::key_callback_(int key, int scancode, int action, int mods)
	{
		(void) scancode;
		auto key_ = magic_enum::enum_cast<Key>(key);
		if(key_.has_value()) {
			KeyEvent e;
			e.key = key_.value();
			e.pressed = action == GLFW_PRESS or action == GLFW_REPEAT;
			e.repeat = action == GLFW_REPEAT;
			e.shift = (mods & GLFW_MOD_SHIFT) != 0;
			e.alt = (mods & GLFW_MOD_ALT) != 0;
			e.control = (mods & GLFW_MOD_CONTROL) != 0;
			e.caps = (mods & GLFW_MOD_CAPS_LOCK) != 0;
			e.num_lock = (mods & GLFW_MOD_NUM_LOCK) != 0;
			key_events.push_back(e);
		}
	}


	bool Window::key_is_down(Key key)
	{
		return glfwGetKey(window_handle, static_cast<int>(key)) == GLFW_PRESS;
	}

	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		Window& w = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		w.mouse_button_callback_(button, action, mods);
	}

	
	void Window::mouse_button_callback_(int button, int action, int mods)
	{
		if(button < 0 or button > 4) {
			return;
		}

		double xpos, ypos;
		glfwGetCursorPos(window_handle, &xpos, &ypos);

		MouseEvent e;
		e.button = magic_enum::enum_cast<MouseButton>(button).value();
		e.pressed = action == GLFW_PRESS;
		e.x = static_cast<int>(xpos);
		e.y = static_cast<int>(ypos);
		e.pressure = get_pressure();
		e.shift = (mods & GLFW_MOD_SHIFT) != 0;
		e.alt = (mods & GLFW_MOD_ALT) != 0;
		e.control = (mods & GLFW_MOD_CONTROL) != 0;
		e.caps = (mods & GLFW_MOD_CAPS_LOCK) != 0;
		e.num_lock = (mods & GLFW_MOD_NUM_LOCK) != 0;
		mouse_events.push_back(e);
	}

	std::pair<int,int> Window::get_mouse_position()
	{
		double xpos, ypos;
		glfwGetCursorPos(window_handle, &xpos, &ypos);
		return {static_cast<int>(xpos), static_cast<int>(ypos)};
	}

	std::tuple<int,int, float> Window::get_mouse_position2()
	{
		auto p = get_mouse_position();
		return {p.first, p.second, get_pressure()};
	}

	bool Window::is_mouse_button_down(int button)
	{
		return glfwGetMouseButton(window_handle, button) == GLFW_PRESS;
	}

	static void scroll_callback(GLFWwindow* window, double x, double y)
	{
		(void)x;
		Window& w = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		w.scroll_callback_(static_cast<int>(y));
	}
	
	void Window::scroll_callback_(int scroll)
	{
		scroll_amount += scroll;
	}

	GLFWwindow* Window::get_handle() const {
		return window_handle;
	}

	void Window::set_title(WindowTitle const& title)
	{
		glfwSetWindowTitle(window_handle, title.c_str());
	}

	void Window::set_dimensions(WindowDimensions dimensions)
	{
		glfwSetWindowSize(window_handle, dimensions.width, dimensions.height);
	}

	WindowDimensions Window::get_dimensions() const
	{
		int w, h;
		glfwGetWindowSize(window_handle, &w, &h);
		return WindowDimensions(w,h);
	}

	bool Window::should_close()
	{
		return glfwWindowShouldClose(window_handle) == GLFW_TRUE;
	}


	Window::~Window()
	{
		if (window_handle) {
			glfwDestroyWindow(window_handle);
			window_handle = nullptr;
		}
	}


}