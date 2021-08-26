#pragma once

// Keys from GLFW/GLFW3.h
// GLFW3 license:

/*
 * Copyright (c) 2002-2006 Marcus Geelnard
 * Copyright (c) 2006-2019 Camilla Löwy <elmindreda@glfw.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would
 *    be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not
 *    be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 *
 *************************************************************************/

typedef enum {

    /* The unknown key */
    PIGEON_WGI_KEY_UNKNOWN =          -1,

    /* Printable keys */
    PIGEON_WGI_KEY_SPACE =            32,
    PIGEON_WGI_KEY_APOSTROPHE =       39,  /* ' */
    PIGEON_WGI_KEY_COMMA =            44,  /* , */
    PIGEON_WGI_KEY_MINUS =            45,  /* - */
    PIGEON_WGI_KEY_PERIOD =           46,  /* . */
    PIGEON_WGI_KEY_SLASH =            47,  /* / */
    PIGEON_WGI_KEY_0 =                48,
    PIGEON_WGI_KEY_1 =                49,
    PIGEON_WGI_KEY_2 =                50,
    PIGEON_WGI_KEY_3 =                51,
    PIGEON_WGI_KEY_4 =                52,
    PIGEON_WGI_KEY_5 =                53,
    PIGEON_WGI_KEY_6 =                54,
    PIGEON_WGI_KEY_7 =                55,
    PIGEON_WGI_KEY_8 =                56,
    PIGEON_WGI_KEY_9 =                57,
    PIGEON_WGI_KEY_SEMICOLON =        59,  /* ; */
    PIGEON_WGI_KEY_EQUAL =            61,  /* = */
    PIGEON_WGI_KEY_A =                65,
    PIGEON_WGI_KEY_B =                66,
    PIGEON_WGI_KEY_C =                67,
    PIGEON_WGI_KEY_D =                68,
    PIGEON_WGI_KEY_E =                69,
    PIGEON_WGI_KEY_F =                70,
    PIGEON_WGI_KEY_G =                71,
    PIGEON_WGI_KEY_H =                72,
    PIGEON_WGI_KEY_I =                73,
    PIGEON_WGI_KEY_J =                74,
    PIGEON_WGI_KEY_K =                75,
    PIGEON_WGI_KEY_L =                76,
    PIGEON_WGI_KEY_M =                77,
    PIGEON_WGI_KEY_N =                78,
    PIGEON_WGI_KEY_O =                79,
    PIGEON_WGI_KEY_P =                80,
    PIGEON_WGI_KEY_Q =                81,
    PIGEON_WGI_KEY_R =                82,
    PIGEON_WGI_KEY_S =                83,
    PIGEON_WGI_KEY_T =                84,
    PIGEON_WGI_KEY_U =                85,
    PIGEON_WGI_KEY_V =                86,
    PIGEON_WGI_KEY_W =                87,
    PIGEON_WGI_KEY_X =                88,
    PIGEON_WGI_KEY_Y =                89,
    PIGEON_WGI_KEY_Z =                90,
    PIGEON_WGI_KEY_LEFT_BRACKET =     91,  /* [ */
    PIGEON_WGI_KEY_BACKSLASH =        92,  /* \ */
    PIGEON_WGI_KEY_RIGHT_BRACKET =    93,  /* ] */
    PIGEON_WGI_KEY_GRAVE_ACCENT =     96,  /* ` */
    PIGEON_WGI_KEY_WORLD_1 =          161, /* non-US #1 */
    PIGEON_WGI_KEY_WORLD_2 =          162, /* non-US #2 */

    /* Function keys */
    PIGEON_WGI_KEY_ESCAPE =           256,
    PIGEON_WGI_KEY_ENTER =            257,
    PIGEON_WGI_KEY_TAB =              258,
    PIGEON_WGI_KEY_BACKSPACE =        259,
    PIGEON_WGI_KEY_INSERT =           260,
    PIGEON_WGI_KEY_DELETE =           261,
    PIGEON_WGI_KEY_RIGHT =            262,
    PIGEON_WGI_KEY_LEFT =             263,
    PIGEON_WGI_KEY_DOWN =             264,
    PIGEON_WGI_KEY_UP =               265,
    PIGEON_WGI_KEY_PAGE_UP =          266,
    PIGEON_WGI_KEY_PAGE_DOWN =        267,
    PIGEON_WGI_KEY_HOME =             268,
    PIGEON_WGI_KEY_END =              269,
    PIGEON_WGI_KEY_CAPS_LOCK =        280,
    PIGEON_WGI_KEY_SCROLL_LOCK =      281,
    PIGEON_WGI_KEY_NUM_LOCK =         282,
    PIGEON_WGI_KEY_PRINT_SCREEN =     283,
    PIGEON_WGI_KEY_PAUSE =            284,
    PIGEON_WGI_KEY_F1 =               290,
    PIGEON_WGI_KEY_F2 =               291,
    PIGEON_WGI_KEY_F3 =               292,
    PIGEON_WGI_KEY_F4 =               293,
    PIGEON_WGI_KEY_F5 =               294,
    PIGEON_WGI_KEY_F6 =               295,
    PIGEON_WGI_KEY_F7 =               296,
    PIGEON_WGI_KEY_F8 =               297,
    PIGEON_WGI_KEY_F9 =               298,
    PIGEON_WGI_KEY_F10 =              299,
    PIGEON_WGI_KEY_F11 =              300,
    PIGEON_WGI_KEY_F12 =              301,
    PIGEON_WGI_KEY_F13 =              302,
    PIGEON_WGI_KEY_F14 =              303,
    PIGEON_WGI_KEY_F15 =              304,
    PIGEON_WGI_KEY_F16 =              305,
    PIGEON_WGI_KEY_F17 =              306,
    PIGEON_WGI_KEY_F18 =              307,
    PIGEON_WGI_KEY_F19 =              308,
    PIGEON_WGI_KEY_F20 =              309,
    PIGEON_WGI_KEY_F21 =              310,
    PIGEON_WGI_KEY_F22 =              311,
    PIGEON_WGI_KEY_F23 =              312,
    PIGEON_WGI_KEY_F24 =              313,
    PIGEON_WGI_KEY_F25 =              314,
    PIGEON_WGI_KEY_KP_0 =             320,
    PIGEON_WGI_KEY_KP_1 =             321,
    PIGEON_WGI_KEY_KP_2 =             322,
    PIGEON_WGI_KEY_KP_3 =             323,
    PIGEON_WGI_KEY_KP_4 =             324,
    PIGEON_WGI_KEY_KP_5 =             325,
    PIGEON_WGI_KEY_KP_6 =             326,
    PIGEON_WGI_KEY_KP_7 =             327,
    PIGEON_WGI_KEY_KP_8 =             328,
    PIGEON_WGI_KEY_KP_9 =             329,
    PIGEON_WGI_KEY_KP_DECIMAL =       330,
    PIGEON_WGI_KEY_KP_DIVIDE =        331,
    PIGEON_WGI_KEY_KP_MULTIPLY =      332,
    PIGEON_WGI_KEY_KP_SUBTRACT =      333,
    PIGEON_WGI_KEY_KP_ADD =           334,
    PIGEON_WGI_KEY_KP_ENTER =         335,
    PIGEON_WGI_KEY_KP_EQUAL =         336,
    PIGEON_WGI_KEY_LEFT_SHIFT =       340,
    PIGEON_WGI_KEY_LEFT_CONTROL =     341,
    PIGEON_WGI_KEY_LEFT_ALT =         342,
    PIGEON_WGI_KEY_LEFT_SUPER =       343,
    PIGEON_WGI_KEY_RIGHT_SHIFT =      344,
    PIGEON_WGI_KEY_RIGHT_CONTROL =    345,
    PIGEON_WGI_KEY_RIGHT_ALT =        346,
    PIGEON_WGI_KEY_RIGHT_SUPER =      347,
    PIGEON_WGI_KEY_MENU =             348

} PigeonWGIKey;
