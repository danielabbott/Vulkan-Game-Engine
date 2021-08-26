#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <pigeon/util.h>
#include <pigeon/wgi/window.h>
#include <pigeon/wgi/input.h>
#include <pigeon/wgi/wgi.h>
#include "singleton.h"

GLFWwindow* pigeon_wgi_glfw_window = NULL; // TODO put this in singleton data as struct GLFWwindow * window_handle
static PigeonWGIKeyCallback key_callback = NULL;
static PigeonWGIMouseButtonCallback mouse_callback = NULL;

void pigeon_wgi_wait_events(void)
{
	glfwWaitEventsTimeout(5);
}

ERROR_RETURN_TYPE pigeon_create_window(PigeonWindowParameters window_parameters)
{
	ASSERT__1(glfwInit() == GLFW_TRUE, "glfwInit() error");
	ASSERT__1(glfwVulkanSupported() == GLFW_TRUE, "Vulkan not supported error");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	if (window_parameters.window_mode == PIGEON_WINDOW_MODE_WINDOWED || 
		window_parameters.window_mode == PIGEON_WINDOW_MODE_MAXIMISED) {

		pigeon_wgi_glfw_window = glfwCreateWindow((int)window_parameters.width, (int)window_parameters.height, 
			window_parameters.title, NULL, NULL);

		if (window_parameters.window_mode == PIGEON_WINDOW_MODE_MAXIMISED) {
			glfwMaximizeWindow(pigeon_wgi_glfw_window);
		}
	}
	else if (window_parameters.window_mode == PIGEON_WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		pigeon_wgi_glfw_window = glfwCreateWindow(mode->width, mode->height, window_parameters.title, monitor, NULL);
	}
	else if (window_parameters.window_mode == PIGEON_WINDOW_MODE_FULLSCREEN) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		pigeon_wgi_glfw_window = glfwCreateWindow(mode->width, mode->height, window_parameters.title, monitor, NULL);
	}

	ASSERT__1(pigeon_wgi_glfw_window, "glfwCreateWindow error");



	return 0;
}

void pigeon_wgi_get_window_dimensions(unsigned int * width, unsigned int * height)
{
	assert(pigeon_wgi_glfw_window && width && height);
	glfwGetFramebufferSize(pigeon_wgi_glfw_window, (int *)width, (int *)height);
}

bool pigeon_wgi_close_requested(void)
{
	if (pigeon_wgi_glfw_window) {
		return glfwWindowShouldClose(pigeon_wgi_glfw_window);
	}
	return false;
}

void pigeon_wgi_poll_events(void)
{
	glfwPollEvents();
}

void pigeon_wgi_destroy_window(void)
{
	if (pigeon_wgi_glfw_window) {
		glfwDestroyWindow(pigeon_wgi_glfw_window);
		pigeon_wgi_glfw_window = NULL;
	}
	glfwTerminate();
}

GLFWwindow* pigeon_wgi_get_glfw_window_handle(void)
{
	assert(pigeon_wgi_glfw_window);
	return pigeon_wgi_glfw_window;
}

uint64_t pigeon_wgi_get_time_micro()
{
	return (uint64_t)(glfwGetTime() * 1000000.0);
}

uint32_t pigeon_wgi_get_time_millis()
{
	return (uint32_t)(glfwGetTime() * 1000.0);
}

float pigeon_wgi_get_time_seconds()
{
	return (float)glfwGetTime();
}

double pigeon_wgi_get_time_seconds_double()
{
	return glfwGetTime();
}

void pigeon_wgi_get_mouse_position(unsigned int * mouse_x, unsigned int * mouse_y)
{
	assert(pigeon_wgi_glfw_window && mouse_x && mouse_y);

	double x,y;
	glfwGetCursorPos(pigeon_wgi_glfw_window, &x, &y);
	
	*mouse_x = (unsigned int)x;
	*mouse_y = (unsigned int)y;
}

bool pigeon_wgi_is_key_down(PigeonWGIKey key)
{
	assert(pigeon_wgi_glfw_window);
	return glfwGetKey(pigeon_wgi_glfw_window, key);
}


static void key_callback_f(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	(void) window;
	(void) scancode;
	(void) mods;

	if(!key_callback) return;

	PigeonWGIKeyEvent e = {
		.key = key,
		.pressed = action == GLFW_PRESS || action == GLFW_REPEAT,
		.repeat = action == GLFW_REPEAT,
		.shift = (mods & GLFW_MOD_SHIFT) != 0,
		.alt = (mods & GLFW_MOD_ALT) != 0,
		.control = (mods & GLFW_MOD_CONTROL) != 0,
		.caps = (mods & GLFW_MOD_CAPS_LOCK) != 0,
		.num_lock = (mods & GLFW_MOD_NUM_LOCK) != 0
	};

	key_callback(e);
}

void pigeon_wgi_set_key_callback(PigeonWGIKeyCallback c)
{
	assert(pigeon_wgi_glfw_window);
	key_callback = c;
	glfwSetKeyCallback(pigeon_wgi_glfw_window, c ? key_callback_f : NULL);
}

static void mouse_callback_f(GLFWwindow* window, int button, int action, int mods)
{
	(void) window;

	if(!mouse_callback || button < 0 || button > 4) return;


	PigeonWGIMouseEvent e = {
		.button = (unsigned int)button,
		.pressed = action == GLFW_PRESS,
		.shift = (mods & GLFW_MOD_SHIFT) != 0,
		.alt = (mods & GLFW_MOD_ALT) != 0,
		.control = (mods & GLFW_MOD_CONTROL) != 0,
		.caps = (mods & GLFW_MOD_CAPS_LOCK) != 0,
		.num_lock = (mods & GLFW_MOD_NUM_LOCK) != 0,
	};

	mouse_callback(e);
}

void pigeon_wgi_set_mouse_button_callback(PigeonWGIMouseButtonCallback c)
{
	assert(pigeon_wgi_glfw_window);
	mouse_callback = c;
	glfwSetMouseButtonCallback(pigeon_wgi_glfw_window, c ? mouse_callback_f : NULL);
}

void pigeon_wgi_set_mouse_grabbed(bool grabbed)
{
	assert(pigeon_wgi_glfw_window);
	glfwSetInputMode(pigeon_wgi_glfw_window, GLFW_CURSOR, grabbed ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);	
}
