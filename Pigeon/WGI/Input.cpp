#include "Input.h"

// ** windows tablet code untested, probably won't event compile **


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#pragma warning(push, 0)
#endif

#define GLFW_INCLUDE_NONE
#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define EASYTAB_IMPLEMENTATION
#include "../deps/easytab.h"

#include "../BetterAssert.h"
#include <spdlog/spdlog.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#pragma warning(pop)
#endif

#ifdef __linux__
#include <optional>
#include <vector>
#endif

using namespace std;

namespace WGI {

#ifdef __linux__
	static ::Display * x_display;
	static ::Window x_window;

	void deinit_tablet()
	{
		if(EasyTab) {
			EasyTab_Unload(x_display);
		}
	}

	void init_tablet(GLFWwindow* window)
	{
		if(EasyTab) {
			return;
		}

		x_display = glfwGetX11Display();
		x_window = glfwGetX11Window(window);
		if(EasyTab_Load(x_display, x_window) != EASYTAB_OK) {
			if(EasyTab) {
				free(EasyTab);
				EasyTab = nullptr;
			}
			return;
		}
	}


	static Bool is_tablet_event (Display *display, XEvent *event, XPointer arg0)
	{
		(void)display;
		(void)arg0;

		if(EasyTab->MotionType == event->type) {
			return true;
		}

		return false;
	}
	
	bool check_tablet_events()
	{
		if(EasyTab) {
			bool found_tablet_events = false;
			XEvent event;
			while(XCheckIfEvent(x_display, &event, is_tablet_event, nullptr)) {
				if (EasyTab_HandleEvent(&event) == EASYTAB_OK) {
					found_tablet_events = true;
					continue;
				}
			}
			return found_tablet_events;
		}
		return false;
	}


#elif defined (WIN32)
	HWND win32_window;


	void init_tablet(GLFWwindow* window)
	{
		if(EasyTab) {
			return;
		}

		win32_window = glfwGetWin32Window(window);

		if(EasyTab_Load(win32_window) != EASYTAB_OK) {
			if(EasyTab) {
				free(EasyTab);
				EasyTab = nullptr;
			}
			return;
		}
	}

	void deinit_tablet()
	{
		if(EasyTab) {
			EasyTab_Unload();
		}
	}
	
	bool check_tablet_events()
	{
		if(EasyTab) {
			bool found_tablet_events = false;
			MSG msg;
			while (PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE)) {
				if (EasyTab_HandleEvent(msg.hwnd, msg.message, msg.lParam, msg.wParam) == EASYTAB_OK) {
					PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);
					found_tablet_events = true;
				}
			}
			return found_tablet_events;
		}
		return false;
	}
#endif


	void poll_input()
	{
		bool tablet_input = check_tablet_events();
		glfwPollEvents();
		tablet_input = check_tablet_events() or tablet_input;
		// TODO might nto need to track whether tablet events have occurred actually.
	}
	void wait_for_input()
	{
		bool tablet_input = check_tablet_events();
		glfwWaitEvents();
		tablet_input = check_tablet_events() or tablet_input;
	}
	
	float get_pressure()
	{
		if(EasyTab) {
			return EasyTab->Pressure;
		}
		return -1;
	}

}