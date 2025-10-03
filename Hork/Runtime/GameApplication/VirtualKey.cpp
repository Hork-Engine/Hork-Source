/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include "VirtualKey.h"

HK_NAMESPACE_BEGIN

const VirtualKeyDesc VirtualKeyTable[] =
{
// Mouse
{ "LBM" },
{ "RBM" },
{ "MBM" },
{ "MB4" },
{ "MB5" },
{ "MB6" },
{ "MB7" },
{ "MB8" },
{ "Wheel Up" },
{ "Wheel Down" },
{ "Wheel Left" },
{ "Wheel Right" },

// Keyboard
{ "Space" },
{ "'" },
{ "," },
{ "-" },
{ "." },
{ "/" },
{ "0" },
{ "1" },
{ "2" },
{ "3" },
{ "4" },
{ "5" },
{ "6" },
{ "7" },
{ "8" },
{ "9" },
{ ";" },
{ "=" },
{ "A" },
{ "B" },
{ "C" },
{ "D" },
{ "E" },
{ "F" },
{ "G" },
{ "H" },
{ "I" },
{ "J" },
{ "K" },
{ "L" },
{ "M" },
{ "N" },
{ "O" },
{ "P" },
{ "Q" },
{ "R" },
{ "S" },
{ "T" },
{ "U" },
{ "V" },
{ "W" },
{ "X" },
{ "Y" },
{ "Z" },
{ "[" },
{ "\\" },
{ "]" },
{ "`" },
{ "Escape" },
{ "Enter" },
{ "Tab" },
{ "Backspace" },
{ "Insert" },
{ "Delete" },
{ "Right" },
{ "Left" },
{ "Down" },
{ "Up" },
{ "Page Up" },
{ "Page Down" },
{ "Home" },
{ "End" },
{ "Caps Lock" },
{ "Scroll Lock" },
{ "Num Lock" },
{ "Print Screen" },
{ "Pause" },
{ "F1" },
{ "F2" },
{ "F3" },
{ "F4" },
{ "F5" },
{ "F6" },
{ "F7" },
{ "F8" },
{ "F9" },
{ "F10" },
{ "F11" },
{ "F12" },
{ "F13" },
{ "F14" },
{ "F15" },
{ "F16" },
{ "F17" },
{ "F18" },
{ "F19" },
{ "F20" },
{ "F21" },
{ "F22" },
{ "F23" },
{ "F24" },
{ "Num 0" },
{ "Num 1" },
{ "Num 2" },
{ "Num 3" },
{ "Num 4" },
{ "Num 5" },
{ "Num 6" },
{ "Num 7" },
{ "Num 8" },
{ "Num 9" },
{ "Num ." },
{ "Num /" },
{ "Num *" },
{ "Num -" },
{ "Num +" },
{ "Num Enter" },
{ "Num =" },
{ "L. Shift" },
{ "L. Ctrl" },
{ "L. Alt" },
{ "L. Super" },
{ "R. Shift" },
{ "R. Ctrl" },
{ "R. Alt" },
{ "R. Super" },
{ "Menu" }
};

const VirtualAxisDesc VirtualAxisTable[] =
{
// Mouse
{"Mouse Horizontal"},
{"Mouse Vertical"}
};

static_assert(VIRTUAL_KEY_COUNT == HK_ARRAY_SIZE(VirtualKeyTable), "Size mismatch");
static_assert(VIRTUAL_AXIS_COUNT == HK_ARRAY_SIZE(VirtualAxisTable), "Size mismatch");

const GamepadKeyDesc GamepadKeyTable[] =
{
{ "Gamepad A" },
{ "Gamepad B" },
{ "Gamepad X" },
{ "Gamepad Y" },
{ "Gamepad Back" },
{ "Gamepad Guide" },
{ "Gamepad Start" },
{ "Gamepad Left Stick" },
{ "Gamepad Right Stick" },
{ "Gamepad Left Shoulder" },
{ "Gamepad Right Shoulder" },
{ "Gamepad DPad Up" },
{ "Gamepad DPad Down" },
{ "Gamepad DPad Left" },
{ "Gamepad DPad Right" },
{ "Gamepad Misc1" },
{ "Gamepad Paddle1" },
{ "Gamepad Paddle2" },
{ "Gamepad Paddle3" },
{ "Gamepad Paddle4" },
{ "Gamepad Touchpad" },
{ "Gamepad Misc2" },
{ "Gamepad Misc3" },
{ "Gamepad Misc4" },
{ "Gamepad Misc5" },
{ "Gamepad Misc6" }
};

const GamepadAxisDesc GamepadAxisTable[] =
{
{ "Gamepad Left X" },
{ "Gamepad Left Y" },
{ "Gamepad Right X" },
{ "Gamepad Right Y" },
{ "Gamepad Trigger Left" },
{ "Gamepad Trigger Right" }
};

static_assert(GAMEPAD_KEY_COUNT == HK_ARRAY_SIZE(GamepadKeyTable), "Size mismatch");
static_assert(GAMEPAD_AXIS_COUNT == HK_ARRAY_SIZE(GamepadAxisTable), "Size mismatch");

HK_NAMESPACE_END
