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

#include "RenderCommon.h"
#include "CanvasRenderer.h"
#include "FrameRenderer.h"
#include "GPUSync.h"
#include "VT/VirtualTextureAnalyzer.h"
#include "VT/VirtualTextureFeedback.h"
#include "VT/VirtualTexturePhysCache.h"

struct SVirtualTextureWorkflow : public RenderCore::IObjectInterface
{
    AVirtualTextureFeedbackAnalyzer FeedbackAnalyzer;
    AVirtualTextureCache PhysCache;

    SVirtualTextureWorkflow( SVirtualTextureCacheCreateInfo const & CreateInfo )
        : PhysCache( CreateInfo )
    {

    }
};

class ARenderBackend : public IRenderBackend
{
public:
    ARenderBackend() : IRenderBackend( "OpenGL 4.5" ) {}

    void Initialize( struct SVideoMode const & _VideoMode ) override;
    void Deinitialize() override;

    void * GetMainWindow() override;

    void RenderFrame( SRenderFrame * pFrameData ) override;
    void SwapBuffers() override;
    void WaitGPU() override;
    void * FenceSync() override;
    void RemoveSync( void * _Sync ) override;
    void WaitSync( void * _Sync ) override;
    void ReadScreenPixels( uint16_t _X, uint16_t _Y, uint16_t _Width, uint16_t _Height, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem ) override;

    void InitializeTexture1D( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) override;
    void InitializeTexture1DArray( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) override;
    void InitializeTexture2D( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height ) override;
    void InitializeTexture2DArray( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _ArraySize ) override;
    void InitializeTexture3D( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _Height, int _Depth ) override;
    void InitializeTextureCubemap( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width ) override;
    void InitializeTextureCubemapArray( TRef< RenderCore::ITexture > * ppTexture, ETexturePixelFormat _PixelFormat, int _NumLods, int _Width, int _ArraySize ) override;
    void WriteTexture( RenderCore::ITexture * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, const void * _SysMem ) override;
    void ReadTexture( RenderCore::ITexture * _Texture, STextureRect const & _Rectangle, ETexturePixelFormat _PixelFormat, size_t _SizeInBytes, unsigned int _Alignment, void * _SysMem ) override;

    void InitializeBuffer( TRef< RenderCore::IBuffer > * ppBuffer, size_t _SizeInBytes ) override;
    void * InitializePersistentMappedBuffer( TRef< RenderCore::IBuffer > * ppBuffer, size_t _SizeInBytes ) override;
    void WriteBuffer( RenderCore::IBuffer * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem ) override;
    void ReadBuffer( RenderCore::IBuffer * _Buffer, size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem ) override;
    void OrphanBuffer( RenderCore::IBuffer * _Buffer ) override;

    void InitializeMaterial( AMaterialGPU * _Material, SMaterialDef const * _Def ) override;

private:
    void SetGPUEvent();
    void RenderView( SRenderView * pRenderView, AFrameGraphTexture ** ppViewTexture );
    void SetViewUniforms();
    void UploadShaderResources();

    TRef< AFrameGraph > FrameGraph;
    AFrameRenderer::SFrameGraphCaptured CapturedResources;

    TRef< ACanvasRenderer > CanvasRenderer;
    TRef< AFrameRenderer > FrameRenderer;

    AGPUSync GPUSync;

public:
    // Just for test
    TRef< SVirtualTextureWorkflow > VTWorkflow;
    AVirtualTexture * TestVT;
};

extern ARenderBackend GRenderBackendLocal;
