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

#include "GameModuleCallback.h"

#include <Platform/Public/BaseTypes.h>

struct SEntryDecl
{
    const char*       GameTitle;
    const char*       RootPath;
    AClassMeta const* ModuleClass;
};

#ifdef AN_OS_WIN32

#    include <Platform/Public/WindowsDefs.h>

/** Runtime entry point */
void RunEngine(SEntryDecl const& _EntryDecl);

#    define AN_ENTRY_DECL(_EntryDecl)                                                                     \
        int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) \
        {                                                                                                 \
            RunEngine(_EntryDecl);                                                                        \
            return 0;                                                                                     \
        }
#    define AN_NO_RUNTIME_MAIN(_MainFunc)                                                                 \
        int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) \
        {                                                                                                 \
            return _MainFunc();                                                                           \
        }

#else

/** Runtime entry point */
void RunEngine(int _Argc, char** _Argv, SEntryDecl const& _EntryDecl);

#    define AN_ENTRY_DECL(_EntryDecl)          \
        int main(int argc, char* argv[])       \
        {                                      \
            RunEngine(argc, argv, _EntryDecl); \
            return 0;                          \
        }
#    define AN_NO_RUNTIME_MAIN(_MainFunc) \
        int main(int argc, char* argv[])  \
        {                                 \
            return _MainFunc();           \
        }

#endif
