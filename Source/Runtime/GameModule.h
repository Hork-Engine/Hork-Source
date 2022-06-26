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

#include "CommandContext.h"

enum CURSOR_MODE
{
    CURSOR_MODE_AUTO,
    CURSOR_MODE_FORCE_ENABLED,
    CURSOR_MODE_FORCE_DISABLED
};

class AGameModule : public ABaseObject
{
    HK_CLASS(AGameModule, ABaseObject)

public:
    /** Quit when user press ESCAPE */
    bool bQuitOnEscape = true;

    /** Toggle fullscreen on ALT+ENTER */
    bool bToggleFullscreenAltEnter = true;

    /** Allow to drop down the console */
    bool bAllowConsole = true;

    CURSOR_MODE CursorMode = CURSOR_MODE_AUTO;

    ACommandContext CommandContext;

    AGameModule();

    virtual void OnGameClose();

    /** Add global console command */
    void AddCommand(AGlobalStringView _Name, TCallback<void(ACommandProcessor const&)> const& _Callback, AGlobalStringView _Comment = ""s);

    /** Remove global console command */
    void RemoveCommand(AStringView _Name);

private:
    void Quit(ACommandProcessor const& _Proc);
    void RebuildMaterials(ACommandProcessor const& _Proc);
};
