/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2020 Alexander Samusev.

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

class ARenderBackend : public IRenderBackend {
public:
    ARenderBackend() : IRenderBackend( "OpenGL 4.5" ) {}

    void Initialize( struct SVideoMode const & _VideoMode ) override;
    void Deinitialize() override;

    void * GetMainWindow() override;

    void RenderFrame( SRenderFrame * _FrameData ) override;
    void SwapBuffers() override;
    void WaitGPU() override;
    void * FenceSync() override;
    void RemoveSync( void * _Sync ) override;
    void WaitSync( void * _Sync ) override;
    void ReadScreenPixels( uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem ) override;

    ATextureGPU * CreateTexture( IGPUResourceOwner * _Owner ) override;
    void DestroyTexture( ATextureGPU * _Texture ) override;
    void InitializeTexture1D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) override;
    void InitializeTexture1DArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) override;
    void InitializeTexture2D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) override;
    void InitializeTexture2DArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) override;
    void InitializeTexture3D( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) override;
    void InitializeTextureCubemap( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) override;
    void InitializeTextureCubemapArray( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) override;
    void InitializeTexture2DNPOT( ATextureGPU * _Texture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) override;
    void WriteTexture( ATextureGPU * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, const void * _SysMem ) override;
    void ReadTexture( ATextureGPU * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem ) override;

    ABufferGPU * CreateBuffer( IGPUResourceOwner * _Owner ) override;
    void DestroyBuffer( ABufferGPU * _Buffer ) override;
    void InitializeBuffer( ABufferGPU * _Buffer, size_t _SizeInBytes ) override;
    void * InitializePersistentMappedBuffer( ABufferGPU * _Buffer, size_t _SizeInBytes ) override;
    void WriteBuffer( ABufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem ) override;
    void ReadBuffer( ABufferGPU * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem ) override;
    void OrphanBuffer( ABufferGPU * _Buffer ) override;

    AMaterialGPU * CreateMaterial( IGPUResourceOwner * _Owner ) override;
    void DestroyMaterial( AMaterialGPU * _Material ) override;
    void InitializeMaterial( AMaterialGPU * _Material, SMaterialBuildData const * _BuildData ) override;

    //size_t AllocateJoints( size_t _JointsCount ) override;
    //void WriteJoints( size_t _Offset, size_t _JointsCount, Float3x4 const * _Matrices ) override;

private:
    void SetGPUEvent();
    void RenderView( SRenderView * _RenderView );

    friend void OpenGL45RenderView( SRenderView * _RenderView );
};

extern ARenderBackend GOpenGL45RenderBackend;

}
