#if defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define EASYTAB_IMPLEMENTATION
#include <easytab.h>

#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#define EASYTAB_IMPLEMENTATION
#include <easytab.h>
#endif

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stdbool.h>
#include <pigeon/assert.h>
#include <pigeon/wgi/window.h>
#include <pigeon/wgi/input.h>
#include <pigeon/wgi/wgi.h>
#include "singleton.h"

GLFWwindow* pigeon_wgi_glfw_window = NULL; // TODO put this in singleton data as struct GLFWwindow * window_handle
static PigeonWGIKeyCallback key_callback = NULL;
static PigeonWGIMouseButtonCallback mouse_callback = NULL;

void pigeon_wgi_wait_events(void)
{
	glfwWaitEventsTimeout(1);
}

static void glfw_error_callback(int code, const char * description)
{
    fprintf(stderr, "GLFW error: %u %s\n", code, description);
}


PIGEON_ERR_RET pigeon_opengl_init(void);

PIGEON_ERR_RET pigeon_create_window(PigeonWindowParameters window_parameters, bool use_opengl)
{
	ASSERT_LOG_R1(glfwInit() == GLFW_TRUE, "glfwInit() error");

	glfwSetErrorCallback(glfw_error_callback);

	if(!use_opengl && !glfwVulkanSupported()) {
		puts("Vulkan not supported");
		return 1;
	}

	if(use_opengl) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_STENCIL_BITS, 0);
		glfwWindowHint(GLFW_DEPTH_BITS, 0);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
		glfwWindowHint(GLFW_SAMPLES, 0);
		glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

		#ifdef DEBUG
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
		#endif
	}
	else {
		// Vulkan

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	}

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

		unsigned int w = window_parameters.width;
		unsigned int h = window_parameters.height;

		if(!w) w = (unsigned)mode->width;
		if(!h) h = (unsigned)mode->height;

		pigeon_wgi_glfw_window = glfwCreateWindow((int)w, (int)h, window_parameters.title, monitor, NULL);
	}
	else if (window_parameters.window_mode == PIGEON_WINDOW_MODE_FULLSCREEN) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		pigeon_wgi_glfw_window = glfwCreateWindow(mode->width, mode->height, window_parameters.title, monitor, NULL);
	}

	if(!pigeon_wgi_glfw_window) {
		printf("glfwCreateWindow error (%s)\n", use_opengl ? "OpenGL" : "Vulkan");
		return 1;
	}

	if(use_opengl) {
		glfwMakeContextCurrent(pigeon_wgi_glfw_window);
		ASSERT_R1(gladLoadGLLoader((GLADloadproc) glfwGetProcAddress));
   		glfwSwapInterval(1);

		if(pigeon_opengl_init()) {
			glfwDestroyWindow(pigeon_wgi_glfw_window);
			glfwTerminate();
			return 1;
		}

	}

	if (glfwRawMouseMotionSupported())
        glfwSetInputMode(pigeon_wgi_glfw_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	if(
		#if defined(__linux__)
			EasyTab_Load(glfwGetX11Display(), glfwGetX11Window(pigeon_wgi_glfw_window))
		#elif defined(_WIN32)
			EasyTab_Load(glfwGetWin32Window(pigeon_wgi_glfw_window))
		#endif
	!= EASYTAB_OK) {
		if(EasyTab) {
			free(EasyTab);
			EasyTab = NULL;
		}
	}

	return 0;
}

void pigeon_wgi_get_window_dimensions(unsigned int * width, unsigned int * height)
{
	assert(pigeon_wgi_glfw_window && width && height);

	if(glfwGetWindowAttrib(pigeon_wgi_glfw_window, GLFW_ICONIFIED) > 0) {
		*width = *height = 0;
	}
	else glfwGetFramebufferSize(pigeon_wgi_glfw_window, (int *)width, (int *)height);
}


PigeonWGISwapchainInfo pigeon_opengl_get_swapchain_info(void)
{
	PigeonWGISwapchainInfo i = {0};
	i.image_count = 3; // assume triple buffering

	pigeon_wgi_get_window_dimensions(&i.width, &i.height);
	
	return i;
}

bool pigeon_wgi_close_requested(void)
{
	if (pigeon_wgi_glfw_window) {
		return glfwWindowShouldClose(pigeon_wgi_glfw_window);
	}
	return false;
}

#if defined(__linux__)

static Bool is_tablet_event (Display *display, XEvent *event, XPointer arg0)
{
	(void)display;
	(void)arg0;

	return event->type == (int)EasyTab->MotionType;
}

static void check_tablet_events(void)
{
	if(EasyTab) {
		XEvent event;
		while(XCheckIfEvent(glfwGetX11Display(), &event, is_tablet_event, NULL)) {
			EasyTab_HandleEvent(&event);
		}
	}
}
#elif defined(_WIN32)
static void check_tablet_events(void)
{
	if(EasyTab) {
		MSG msg;
		while (PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE) &&
			EasyTab_HandleEvent(msg.hwnd, msg.message, msg.lParam, msg.wParam) == EASYTAB_OK) 
		{
			PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);
		}
	}
}
#endif

void pigeon_wgi_poll_events(void)
{
	check_tablet_events();
	glfwPollEvents();
	check_tablet_events();
}

float pigeon_wgi_get_stylus_pressure(void)
{
	return EasyTab ? EasyTab->Pressure : -1;
}

void pigeon_wgi_destroy_window(void)
{
#ifdef EASYTAB_IMPLEMENTATION
	if(EasyTab) {
#if defined(__linux__)
		EasyTab_Unload(glfwGetX11Display());
#elif defined(_WIN32)
		EasyTab_Unload();
#endif
	}
#endif

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

void pigeon_wgi_get_mouse_position(int * mouse_x, int * mouse_y)
{
	assert(pigeon_wgi_glfw_window && mouse_x && mouse_y);

	double x,y;
	glfwGetCursorPos(pigeon_wgi_glfw_window, &x, &y);
	
	*mouse_x = (int)x;
	*mouse_y = (int)y;
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

void pigeon_wgi_set_cursor_type(PigeonWGICursorType type)
{
	assert(pigeon_wgi_glfw_window);

	switch(type) {
		case PIGEON_WGI_CURSOR_TYPE_NORMAL:
			glfwSetInputMode(pigeon_wgi_glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);	
   			glfwSetInputMode(pigeon_wgi_glfw_window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
			break;
		case PIGEON_WGI_CURSOR_TYPE_FPS_CAMERA:
			glfwSetInputMode(pigeon_wgi_glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			if (glfwRawMouseMotionSupported())
				glfwSetInputMode(pigeon_wgi_glfw_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);	
			break;
		case PIGEON_WGI_CURSOR_TYPE_CUSTOM:
			glfwSetInputMode(pigeon_wgi_glfw_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);	
   			glfwSetInputMode(pigeon_wgi_glfw_window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
			break;
	}
}

void pigeon_wgi_swap_buffers(void)
{
	glfwSwapBuffers(pigeon_wgi_glfw_window);
}