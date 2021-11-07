#pragma once

#include <stdbool.h>

typedef enum {
	PIGEON_WINDOW_MODE_WINDOWED,
	PIGEON_WINDOW_MODE_MAXIMISED,
	PIGEON_WINDOW_MODE_FULLSCREEN,
	PIGEON_WINDOW_MODE_EXCLUSIVE_FULLSCREEN
} PigeonWindowMode;

typedef struct PigeonWindowParameters {
	unsigned int width, height;
	const char* title; /* Valid for call to initialise_engine() */
	PigeonWindowMode window_mode;
} PigeonWindowParameters;

struct GLFWwindow* pigeon_wgi_get_glfw_window_handle(void);

void pigeon_wgi_get_window_dimensions(unsigned int * width, unsigned int * height);

void pigeon_wgi_poll_events(void);
bool pigeon_wgi_close_requested(void);

void pigeon_wgi_destroy_window(void);
