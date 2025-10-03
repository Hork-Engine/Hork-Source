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

/*

Usage: Somewhere in the end of your Main file

// Declare application class, e.g:
using ApplicationClass = YourApplicationClassName;
#include "EntryPoint.h"

Your application class should have Initialize and Deinitialize methods. It can be empty if you want to leave
all your initialization code in the constructor.

class YourApplicationClassName : public Hk::GameApplication
{
public:
    YourApplicationClassName(ArgumentPack const& args) :
        Hk::GameApplication(args, "Your title")
    {}

    void Initialize()
    {
    }

    void Deinitialize()
    {
    }
};

*/

alignas(alignof(ApplicationClass)) static char AppData[sizeof(ApplicationClass)];

#ifdef HK_OS_WIN32
#    include <Hork/Core/WindowsDefs.h>
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char* argv[])
#endif
{
    using namespace Hk;

#if defined(HK_DEBUG) && defined(HK_COMPILER_MSVC)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef HK_OS_WIN32
    ArgumentPack args;
#else
    ArgumentPack args(argc, argv);
#endif

    ApplicationClass* app = new (AppData) ApplicationClass(args);
    app->Initialize();
    app->RunMainLoop();
    app->Deinitialize();
    int exitCode = app->ExitCode();
    app->~ApplicationClass();
    return exitCode;
}
