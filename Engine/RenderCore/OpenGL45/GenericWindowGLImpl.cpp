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

#include "GenericWindowGLImpl.h"
#include "DeviceGLImpl.h"

#include <Engine/Core/Platform.h>
#include <Engine/Core/WindowsDefs.h>
#include <Engine/Core/BaseMath.h>

#include "GL/glew.h"
#ifdef HK_OS_WIN32
#    include "GL/wglew.h"
#endif
#ifdef HK_OS_LINUX
#    include "GL/glxew.h"
#endif

#include <SDL3/SDL.h>

HK_NAMESPACE_BEGIN

namespace RenderCore
{

GenericWindowGLImpl::GenericWindowGLImpl(DeviceGLImpl* pDevice, WindowSettings const& windowSettings, WindowPoolGL& windowPool, WindowPoolGL::WindowGL windowHandle) :
    IGenericWindow(pDevice),
    WindowPool(windowPool),
    bUseExternalHandle(windowHandle.Handle != nullptr)
{
    if (bUseExternalHandle)
    {
        WindowGL = windowHandle;
    }
    else
    {
        WindowGL = WindowPool.Create();
        if (!WindowGL.ImmediateCtx)
        {
            WindowGL.ImmediateCtx = new ImmediateContextGLImpl(pDevice, WindowGL, false);
        }
    }

    SetHandle(WindowGL.Handle);

    SDL_SetPointerProperty(SDL_GetWindowProperties(WindowGL.Handle), "p", this);

    ChangeWindowSettings(windowSettings);
}

GenericWindowGLImpl::~GenericWindowGLImpl()
{
    SDL_SetPointerProperty(SDL_GetWindowProperties(WindowGL.Handle), "p", nullptr);

    if (bUseExternalHandle)
    {
        SDL_HideWindow(WindowGL.Handle);
    }
    else
    {
        if (WindowGL.ImmediateCtx)
        {
            WindowGL.ImmediateCtx->RemoveRef();
            WindowGL.ImmediateCtx = nullptr;
        }

        WindowPool.Destroy(WindowGL);
    }
}

void GenericWindowGLImpl::SetSwapChain(ISwapChain* InSwapChain)
{
    m_SwapChain = InSwapChain;
}

} // namespace RenderCore

HK_NAMESPACE_END
