/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

enum ECursorMode
{
    CURSOR_MODE_AUTO,
    CURSOR_MODE_FORCE_ENABLED,
    CURSOR_MODE_FORCE_DISABLED
};

class IGameModule : public ABaseObject
{
    AN_CLASS(IGameModule, ABaseObject)

public:
    /** Quit when user press ESCAPE */
    bool bQuitOnEscape = true;

    /** Toggle fullscreen on ALT+ENTER */
    bool bToggleFullscreenAltEnter = true;

    /** Allow to drop down the console */
    bool bAllowConsole = true;

    ECursorMode CursorMode = CURSOR_MODE_AUTO;

    ACommandContext CommandContext;

    IGameModule();

    virtual void OnGameClose();

    /** Add global console command */
    void AddCommand(const char* _Name, TCallback<void(ARuntimeCommandProcessor const&)> const& _Callback, const char* _Comment = "");

    /** Remove global console command */
    void RemoveCommand(const char* _Name);

private:
    void Quit(ARuntimeCommandProcessor const& _Proc);
    void RebuildMaterials(ARuntimeCommandProcessor const& _Proc);
};
