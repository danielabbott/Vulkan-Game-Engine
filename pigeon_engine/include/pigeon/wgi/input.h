#pragma once

#include "keys.h"
#include <stdbool.h>

bool pigeon_wgi_is_key_down(PigeonWGIKey);
void pigeon_wgi_get_mouse_position(int* mouse_x, int* mouse_y);
float pigeon_wgi_get_stylus_pressure(void);

typedef enum {
	PIGEON_WGI_CURSOR_TYPE_NORMAL,
	PIGEON_WGI_CURSOR_TYPE_FPS_CAMERA,
	PIGEON_WGI_CURSOR_TYPE_CUSTOM // Hides cursor for custom cursor rendering
} PigeonWGICursorType;

void pigeon_wgi_set_cursor_type(PigeonWGICursorType);

typedef struct PigeonWGIKeyEvent {
	PigeonWGIKey key;

	bool pressed;
	bool repeat; // ignore if pressed = false
	bool shift;
	bool alt;
	bool control;

	bool caps;
	bool num_lock;
} PigeonWGIKeyEvent;

typedef enum {
	PIGEON_WGI_MOUSE_BUTTON_LEFT,
	PIGEON_WGI_MOUSE_BUTTON_RIGHT,
	PIGEON_WGI_MOUSE_BUTTON_MIDDLE,
	PIGEON_WGI_MOUSE_BUTTON_BACK,
	PIGEON_WGI_MOUSE_BUTTON_FORWARD
} PigeonWGIMouseButton;

typedef struct PigeonWGIMouseEvent {
	PigeonWGIMouseButton button;
	bool pressed;

	bool shift;
	bool alt;
	bool control;
	bool caps;
	bool num_lock;
} PigeonWGIMouseEvent;

typedef void (*PigeonWGIKeyCallback)(PigeonWGIKeyEvent);
typedef void (*PigeonWGIMouseButtonCallback)(PigeonWGIMouseEvent);

void pigeon_wgi_set_key_callback(PigeonWGIKeyCallback);
void pigeon_wgi_set_mouse_button_callback(PigeonWGIMouseButtonCallback);
