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

#include <Platform/BaseTypes.h>

enum EWidgetStyle
{
    WIDGET_STYLE_DEFAULT     = 0,
    WIDGET_STYLE_BACKGROUND  = AN_BIT(0),                           // widget is stays background
    WIDGET_STYLE_FOREGROUND  = AN_BIT(1),                           // widget is stays on top
    WIDGET_STYLE_EXCLUSIVE   = WIDGET_STYLE_FOREGROUND | AN_BIT(2), // widget is stays on top of other widgets and receives inputs
    WIDGET_STYLE_RESIZABLE   = AN_BIT(3),                           // allows user to change size/maximize window
    WIDGET_STYLE_NO_INPUTS   = AN_BIT(4),                           // forward inputs to parent
    WIDGET_STYLE_TRANSPARENT = AN_BIT(6),                           // transparent for clicking/hovering
    WIDGET_STYLE_POPUP       = WIDGET_STYLE_FOREGROUND | AN_BIT(5)
};

enum EWidgetAlignment
{
    WIDGET_ALIGNMENT_NONE,
    WIDGET_ALIGNMENT_LEFT,
    WIDGET_ALIGNMENT_RIGHT,
    WIDGET_ALIGNMENT_TOP,
    WIDGET_ALIGNMENT_BOTTOM,
    WIDGET_ALIGNMENT_CENTER,
    WIDGET_ALIGNMENT_STRETCH
};

enum EWidgetLayout
{
    WIDGET_LAYOUT_EXPLICIT,
    WIDGET_LAYOUT_GRID,
    WIDGET_LAYOUT_HORIZONTAL,
    WIDGET_LAYOUT_HORIZONTAL_WRAP,
    WIDGET_LAYOUT_VERTICAL,
    WIDGET_LAYOUT_VERTICAL_WRAP,
    WIDGET_LAYOUT_IMAGE,
    WIDGET_LAYOUT_CUSTOM
};

enum EWidgetVisibility
{
    WIDGET_VISIBILITY_VISIBLE,   // The widget will appear normally
    WIDGET_VISIBILITY_INVISIBLE, // The widget will be not visible, but will take up space in the layout
    WIDGET_VISIBILITY_COLLAPSED  // The widget will be not visible and will take no space in the layout
};
