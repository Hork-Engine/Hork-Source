/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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
{ "Menu" },

// Joystick
{ "JoyBtn1" },
{ "JoyBtn2" },
{ "JoyBtn3" },
{ "JoyBtn4" },
{ "JoyBtn5" },
{ "JoyBtn6" },
{ "JoyBtn7" },
{ "JoyBtn8" },
{ "JoyBtn9" },
{ "JoyBtn10" },
{ "JoyBtn11" },
{ "JoyBtn12" },
{ "JoyBtn13" },
{ "JoyBtn14" },
{ "JoyBtn15" },
{ "JoyBtn16" },
{ "JoyBtn17" },
{ "JoyBtn18" },
{ "JoyBtn19" },
{ "JoyBtn20" },
{ "JoyBtn21" },
{ "JoyBtn22" },
{ "JoyBtn23" },
{ "JoyBtn24" },
{ "JoyBtn25" },
{ "JoyBtn26" },
{ "JoyBtn27" },
{ "JoyBtn28" },
{ "JoyBtn29" },
{ "JoyBtn30" },
{ "JoyBtn31" },
{ "JoyBtn32" }
};

const VirtualAxisDesc VirtualAxisTable[] =
{
// Mouse
{"Mouse Horizontal"},
{"Mouse Vertical"},

// Joystick
{"JoyAxis1"},
{"JoyAxis2"},
{"JoyAxis3"},
{"JoyAxis4"},
{"JoyAxis5"},
{"JoyAxis6"},
{"JoyAxis7"},
{"JoyAxis8"},
{"JoyAxis9"},
{"JoyAxis10"},
{"JoyAxis11"},
{"JoyAxis12"},
{"JoyAxis13"},
{"JoyAxis14"},
{"JoyAxis15"},
{"JoyAxis16"},
{"JoyAxis17"},
{"JoyAxis18"},
{"JoyAxis19"},
{"JoyAxis20"},
{"JoyAxis21"},
{"JoyAxis22"},
{"JoyAxis23"},
{"JoyAxis24"},
{"JoyAxis25"},
{"JoyAxis26"},
{"JoyAxis27"},
{"JoyAxis28"},
{"JoyAxis29"},
{"JoyAxis30"},
{"JoyAxis31"},
{"JoyAxis32"}
};

static_assert(VirtualKeyTableSize == HK_ARRAY_SIZE(VirtualKeyTable), "Size mismatch");
static_assert(VirtualAxisTableSize == HK_ARRAY_SIZE(VirtualAxisTable), "Size mismatch");

HK_NAMESPACE_END
