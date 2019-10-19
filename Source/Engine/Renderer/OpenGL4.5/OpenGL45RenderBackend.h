/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

#include "OpenGL45Common.h"

namespace OpenGL45 {

class FRenderBackend : public IRenderBackend {
public:
    FRenderBackend() : IRenderBackend( "OpenGL 4.5" ) {}

    void PreInit() override;
    void Initialize( void * _NativeWindowHandle ) override;
    void Deinitialize() override;

    void RenderFrame( FRenderFrame * _FrameData ) override;
    void WaitGPU() override;

    FTextureGPU * CreateTexture( IGPUResourceOwner * _Owner ) override;
    void DestroyTexture( FTextureGPU * _Texture ) override;
    void InitializeTexture1D( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) override;
    void InitializeTexture1DArray( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) override;
    void InitializeTexture2D( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) override;
    void InitializeTexture2DArray( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) override;
    void InitializeTexture3D( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) override;
    void InitializeTextureCubemap( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) override;
    void InitializeTextureCubemapArray( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) override;
    void InitializeTexture2DNPOT( FTextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) override;
    void WriteTexture( FTextureGPU * _Texture, FTextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, const void * _SysMem ) override;
    void ReadTexture( FTextureGPU * _Texture, FTextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem ) override;

    FBufferGPU * CreateBuffer( IGPUResourceOwner * _Owner ) override;
    void DestroyBuffer( FBufferGPU * _Buffer ) override;
    void InitializeBuffer( FBufferGPU * _Buffer, size_t _SizeInBytes, bool _DynamicStorage ) override;
    void WriteBuffer( FBufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem ) override;
    void ReadBuffer( FBufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem ) override;

    FMaterialGPU * CreateMaterial( IGPUResourceOwner * _Owner ) override;
    void DestroyMaterial( FMaterialGPU * _Material ) override;
    void InitializeMaterial( FMaterialGPU * _Material, FMaterialBuildData const * _BuildData ) override;

    size_t AllocateJoints( size_t _JointsCount ) override;
    void WriteJoints( size_t _Offset, size_t _JointsCount, Float3x4 const * _Matrices ) override;

private:
    void SetGPUEvent();
    void SwapBuffers();
    void RenderView( FRenderView * _RenderView );

    friend void OpenGL45RenderView( FRenderView * _RenderView );
};

extern FRenderBackend GOpenGL45RenderBackend;

}
