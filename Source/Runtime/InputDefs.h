/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#pragma once

/* Input limits */
constexpr int MAX_KEYBOARD_BUTTONS = 349;
constexpr int MAX_MODIFIERS        = 6;
constexpr int MAX_MOUSE_BUTTONS    = 12;
constexpr int MAX_MOUSE_AXES       = 2;
constexpr int MAX_JOYSTICKS_COUNT  = 16;
constexpr int MAX_JOYSTICK_BUTTONS = 32;
constexpr int MAX_JOYSTICK_AXES    = 32;

enum {
    /* The unknown key */
    //KEY_UNKNOWN            = -1,

    /* Printable keys */
    KEY_SPACE              = 32,
    KEY_APOSTROPHE         = 39,  /* ' */
    KEY_COMMA              = 44,  /* , */
    KEY_MINUS              = 45,  /* - */
    KEY_PERIOD             = 46,  /* . */
    KEY_SLASH              = 47,  /* / */
    KEY_0                  = 48,
    KEY_1                  = 49,
    KEY_2                  = 50,
    KEY_3                  = 51,
    KEY_4                  = 52,
    KEY_5                  = 53,
    KEY_6                  = 54,
    KEY_7                  = 55,
    KEY_8                  = 56,
    KEY_9                  = 57,
    KEY_SEMICOLON          = 59,  /* ; */
    KEY_EQUAL              = 61,  /* = */
    KEY_A                  = 65,
    KEY_B                  = 66,
    KEY_C                  = 67,
    KEY_D                  = 68,
    KEY_E                  = 69,
    KEY_F                  = 70,
    KEY_G                  = 71,
    KEY_H                  = 72,
    KEY_I                  = 73,
    KEY_J                  = 74,
    KEY_K                  = 75,
    KEY_L                  = 76,
    KEY_M                  = 77,
    KEY_N                  = 78,
    KEY_O                  = 79,
    KEY_P                  = 80,
    KEY_Q                  = 81,
    KEY_R                  = 82,
    KEY_S                  = 83,
    KEY_T                  = 84,
    KEY_U                  = 85,
    KEY_V                  = 86,
    KEY_W                  = 87,
    KEY_X                  = 88,
    KEY_Y                  = 89,
    KEY_Z                  = 90,
    KEY_LEFT_BRACKET       = 91,  /* [ */
    KEY_BACKSLASH          = 92,  /* \ */
    KEY_RIGHT_BRACKET      = 93,  /* ] */
    KEY_GRAVE_ACCENT       = 96,  /* ` */

    /* Function keys */
    KEY_ESCAPE             = 256,
    KEY_ENTER              = 257,
    KEY_TAB                = 258,
    KEY_BACKSPACE          = 259,
    KEY_INSERT             = 260,
    KEY_DELETE             = 261,
    KEY_RIGHT              = 262,
    KEY_LEFT               = 263,
    KEY_DOWN               = 264,
    KEY_UP                 = 265,
    KEY_PAGE_UP            = 266,
    KEY_PAGE_DOWN          = 267,
    KEY_HOME               = 268,
    KEY_END                = 269,
    KEY_CAPS_LOCK          = 280,
    KEY_SCROLL_LOCK        = 281,
    KEY_NUM_LOCK           = 282,
    KEY_PRINT_SCREEN       = 283,
    KEY_PAUSE              = 284,
    KEY_F1                 = 290,
    KEY_F2                 = 291,
    KEY_F3                 = 292,
    KEY_F4                 = 293,
    KEY_F5                 = 294,
    KEY_F6                 = 295,
    KEY_F7                 = 296,
    KEY_F8                 = 297,
    KEY_F9                 = 298,
    KEY_F10                = 299,
    KEY_F11                = 300,
    KEY_F12                = 301,
    KEY_F13                = 302,
    KEY_F14                = 303,
    KEY_F15                = 304,
    KEY_F16                = 305,
    KEY_F17                = 306,
    KEY_F18                = 307,
    KEY_F19                = 308,
    KEY_F20                = 309,
    KEY_F21                = 310,
    KEY_F22                = 311,
    KEY_F23                = 312,
    KEY_F24                = 313,
    KEY_KP_0               = 320,
    KEY_KP_1               = 321,
    KEY_KP_2               = 322,
    KEY_KP_3               = 323,
    KEY_KP_4               = 324,
    KEY_KP_5               = 325,
    KEY_KP_6               = 326,
    KEY_KP_7               = 327,
    KEY_KP_8               = 328,
    KEY_KP_9               = 329,
    KEY_KP_DECIMAL         = 330,
    KEY_KP_DIVIDE          = 331,
    KEY_KP_MULTIPLY        = 332,
    KEY_KP_SUBTRACT        = 333,
    KEY_KP_ADD             = 334,
    KEY_KP_ENTER           = 335,
    KEY_KP_EQUAL           = 336,
    KEY_LEFT_SHIFT         = 340,
    KEY_LEFT_CONTROL       = 341,
    KEY_LEFT_ALT           = 342,
    KEY_LEFT_SUPER         = 343,
    KEY_RIGHT_SHIFT        = 344,
    KEY_RIGHT_CONTROL      = 345,
    KEY_RIGHT_ALT          = 346,
    KEY_RIGHT_SUPER        = 347,
    KEY_MENU               = 348,
    KEY_LAST               = KEY_MENU,

    /* Modifiers */
    KEY_MOD_SHIFT              = 0,
    KEY_MOD_CONTROL = 1,
    KEY_MOD_ALT     = 2,
    KEY_MOD_SUPER   = 3,
    KEY_MOD_CAPS_LOCK = 4,
    KEY_MOD_NUM_LOCK  = 5,
    KEY_MOD_LAST      = KEY_MOD_NUM_LOCK,

    MOD_MASK_SHIFT     = 1 << KEY_MOD_SHIFT,
    MOD_MASK_CONTROL   = 1 << KEY_MOD_CONTROL,
    MOD_MASK_ALT       = 1 << KEY_MOD_ALT,
    MOD_MASK_SUPER     = 1 << KEY_MOD_SUPER,
    MOD_MASK_CAPS_LOCK = 1 << KEY_MOD_CAPS_LOCK,
    MOD_MASK_NUM_LOCK  = 1 << KEY_MOD_NUM_LOCK,

    #define HAS_MODIFIER( _ModifierMask, _Modifier )  ( !!( (_ModifierMask) & ( 1 << (_Modifier) ) ) )

    /* Mouse buttons */
    MOUSE_BUTTON_BASE     = 0,
    MOUSE_BUTTON_1        = (MOUSE_BUTTON_BASE + 0),
    MOUSE_BUTTON_2        = (MOUSE_BUTTON_BASE + 1),
    MOUSE_BUTTON_3        = (MOUSE_BUTTON_BASE + 2),
    MOUSE_BUTTON_4        = (MOUSE_BUTTON_BASE + 3),
    MOUSE_BUTTON_5        = (MOUSE_BUTTON_BASE + 4),
    MOUSE_BUTTON_6        = (MOUSE_BUTTON_BASE + 5),
    MOUSE_BUTTON_7        = (MOUSE_BUTTON_BASE + 6),
    MOUSE_BUTTON_8        = (MOUSE_BUTTON_BASE + 7),
    MOUSE_WHEEL_UP        = (MOUSE_BUTTON_BASE + 8),
    MOUSE_WHEEL_DOWN      = (MOUSE_BUTTON_BASE + 9),
    MOUSE_WHEEL_LEFT      = (MOUSE_BUTTON_BASE + 10),
    MOUSE_WHEEL_RIGHT     = (MOUSE_BUTTON_BASE + 11),
    MOUSE_BUTTON_LAST     = MOUSE_WHEEL_RIGHT,
    MOUSE_BUTTON_LEFT     = MOUSE_BUTTON_1,
    MOUSE_BUTTON_RIGHT    = MOUSE_BUTTON_2,
    MOUSE_BUTTON_MIDDLE   = MOUSE_BUTTON_3,

    MOUSE_AXIS_BASE       = (MOUSE_BUTTON_LAST + 1),
    MOUSE_AXIS_X          = (MOUSE_AXIS_BASE + 0),
    MOUSE_AXIS_Y          = (MOUSE_AXIS_BASE + 1),
    MOUSE_AXIS_LAST       = MOUSE_AXIS_Y,

    /* Joystick buttons */
    JOY_BUTTON_BASE       = 0,
    JOY_BUTTON_1          = (JOY_BUTTON_BASE + 0),
    JOY_BUTTON_2          = (JOY_BUTTON_BASE + 1),
    JOY_BUTTON_3          = (JOY_BUTTON_BASE + 2),
    JOY_BUTTON_4          = (JOY_BUTTON_BASE + 3),
    JOY_BUTTON_5          = (JOY_BUTTON_BASE + 4),
    JOY_BUTTON_6          = (JOY_BUTTON_BASE + 5),
    JOY_BUTTON_7          = (JOY_BUTTON_BASE + 6),
    JOY_BUTTON_8          = (JOY_BUTTON_BASE + 7),
    JOY_BUTTON_9          = (JOY_BUTTON_BASE + 8),
    JOY_BUTTON_10         = (JOY_BUTTON_BASE + 9),
    JOY_BUTTON_11         = (JOY_BUTTON_BASE + 10),
    JOY_BUTTON_12         = (JOY_BUTTON_BASE + 11),
    JOY_BUTTON_13         = (JOY_BUTTON_BASE + 12),
    JOY_BUTTON_14         = (JOY_BUTTON_BASE + 13),
    JOY_BUTTON_15         = (JOY_BUTTON_BASE + 14),
    JOY_BUTTON_16         = (JOY_BUTTON_BASE + 15),
    JOY_BUTTON_17         = (JOY_BUTTON_BASE + 16),
    JOY_BUTTON_18         = (JOY_BUTTON_BASE + 17),
    JOY_BUTTON_19         = (JOY_BUTTON_BASE + 18),
    JOY_BUTTON_20         = (JOY_BUTTON_BASE + 19),
    JOY_BUTTON_21         = (JOY_BUTTON_BASE + 20),
    JOY_BUTTON_22         = (JOY_BUTTON_BASE + 21),
    JOY_BUTTON_23         = (JOY_BUTTON_BASE + 22),
    JOY_BUTTON_24         = (JOY_BUTTON_BASE + 23),
    JOY_BUTTON_25         = (JOY_BUTTON_BASE + 24),
    JOY_BUTTON_26         = (JOY_BUTTON_BASE + 25),
    JOY_BUTTON_27         = (JOY_BUTTON_BASE + 26),
    JOY_BUTTON_28         = (JOY_BUTTON_BASE + 27),
    JOY_BUTTON_29         = (JOY_BUTTON_BASE + 28),
    JOY_BUTTON_30         = (JOY_BUTTON_BASE + 29),
    JOY_BUTTON_31         = (JOY_BUTTON_BASE + 30),
    JOY_BUTTON_32         = (JOY_BUTTON_BASE + 31),
    JOY_BUTTON_LAST       = JOY_BUTTON_32,

    /* Joystick axes */
    JOY_AXIS_BASE         = (JOY_BUTTON_LAST + 1),
    JOY_AXIS_1            = (JOY_AXIS_BASE + 0),
    JOY_AXIS_2            = (JOY_AXIS_BASE + 1),
    JOY_AXIS_3            = (JOY_AXIS_BASE + 2),
    JOY_AXIS_4            = (JOY_AXIS_BASE + 3),
    JOY_AXIS_5            = (JOY_AXIS_BASE + 4),
    JOY_AXIS_6            = (JOY_AXIS_BASE + 5),
    JOY_AXIS_7            = (JOY_AXIS_BASE + 6),
    JOY_AXIS_8            = (JOY_AXIS_BASE + 7),
    JOY_AXIS_9            = (JOY_AXIS_BASE + 8),
    JOY_AXIS_10           = (JOY_AXIS_BASE + 9),
    JOY_AXIS_11           = (JOY_AXIS_BASE + 10),
    JOY_AXIS_12           = (JOY_AXIS_BASE + 11),
    JOY_AXIS_13           = (JOY_AXIS_BASE + 12),
    JOY_AXIS_14           = (JOY_AXIS_BASE + 13),
    JOY_AXIS_15           = (JOY_AXIS_BASE + 14),
    JOY_AXIS_16           = (JOY_AXIS_BASE + 15),
    JOY_AXIS_17           = (JOY_AXIS_BASE + 16),
    JOY_AXIS_18           = (JOY_AXIS_BASE + 17),
    JOY_AXIS_19           = (JOY_AXIS_BASE + 18),
    JOY_AXIS_20           = (JOY_AXIS_BASE + 19),
    JOY_AXIS_21           = (JOY_AXIS_BASE + 20),
    JOY_AXIS_22           = (JOY_AXIS_BASE + 21),
    JOY_AXIS_23           = (JOY_AXIS_BASE + 22),
    JOY_AXIS_24           = (JOY_AXIS_BASE + 23),
    JOY_AXIS_25           = (JOY_AXIS_BASE + 24),
    JOY_AXIS_26           = (JOY_AXIS_BASE + 25),
    JOY_AXIS_27           = (JOY_AXIS_BASE + 26),
    JOY_AXIS_28           = (JOY_AXIS_BASE + 27),
    JOY_AXIS_29           = (JOY_AXIS_BASE + 28),
    JOY_AXIS_30           = (JOY_AXIS_BASE + 29),
    JOY_AXIS_31           = (JOY_AXIS_BASE + 30),
    JOY_AXIS_32           = (JOY_AXIS_BASE + 31),
    JOY_AXIS_LAST         = JOY_AXIS_32,

    KEY_UNDEFINED = 0xffff
};

enum EInputAction
{
    IA_RELEASE,
    IA_PRESS,
    IA_REPEAT
};
