/*

Graphics Hardware Interface (GHI) is part of Angie Engine Source Code

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

#include "GHIPipeline.h"
#include "GHICommandBuffer.h"

#include <Engine/Core/Public/Hash.h>
#include <Engine/Core/Public/PodArray.h>

namespace GHI {

enum CLIP_CONTROL : uint8_t {
    CLIP_CONTROL_OPENGL,
    CLIP_CONTROL_DIRECTX
};

enum VIEWPORT_ORIGIN : uint8_t {
    VIEWPORT_ORIGIN_TOP_LEFT,
    VIEWPORT_ORIGIN_BOTTOM_LEFT
};

struct StateCreateInfo {
    CLIP_CONTROL ClipControl;
    VIEWPORT_ORIGIN ViewportOrigin;     /// Viewport and Scissor origin
};

/// Hardware state
class State final : public NonCopyable, IObjectInterface {

    friend class CommandBuffer;
    friend class Framebuffer;
    friend class Pipeline;
    friend class RenderPass;
    friend class Texture;
    friend class TransformFeedback;
    friend class QueryPool;

public:
    State();

    void Initialize( Device * _Device, StateCreateInfo const & _CreateInfo );
    void Deinitialize();

    void SetSwapChainResolution( int _Width, int _Height );

    CLIP_CONTROL GetClipControl() const { return ClipControl; }

    VIEWPORT_ORIGIN GetViewportOrigin() const { return ViewportOrigin; }

    Device * GetDevice() { return pDevice; }

private:
    void PolygonOffsetClampSafe( float _Slope, int _Bias, float _Clamp );
    void PackAlignment( unsigned int _Alignment );
    void UnpackAlignment( unsigned int _Alignment );
    void ClampReadColor( COLOR_CLAMP _ColorClamp );
    struct VertexArrayObject * CachedVAO( VertexBindingInfo const * pVertexBindings,
                                          uint32_t NumVertexBindings,
                                          VertexAttribInfo const * pVertexAttribs,
                                          uint32_t NumVertexAttribs );

    Device *                  pDevice;

    CLIP_CONTROL              ClipControl;
    VIEWPORT_ORIGIN           ViewportOrigin;

    unsigned int *            TmpHandles;
    ptrdiff_t *               TmpPointers;
    ptrdiff_t *               TmpPointers2;

    unsigned int              BufferBindings[MAX_BUFFER_SLOTS];
    unsigned int              SampleBindings[MAX_SAMPLER_SLOTS];
    unsigned int              TextureBindings[MAX_SAMPLER_SLOTS];

    Pipeline *                CurrentPipeline;
    struct VertexArrayObject * CurrentVAO;
    uint8_t                   NumPatchVertices;       // count of patch vertices to set by glPatchParameteri

    struct {
        unsigned int          PackAlignment;
        unsigned int          UnpackAlignment;
    } PixelStore;

    // current binding state
    struct {
        unsigned int          ReadFramebuffer;
        unsigned int          DrawFramebuffer;
        unsigned short        DrawFramebufferWidth;
        unsigned short        DrawFramebufferHeight;
        unsigned int          DrawInderectBuffer;
        unsigned int          DispatchIndirectBuffer;
        BlendingStateInfo const *     BlendState;             // current blend state binding
        RasterizerStateInfo const *   RasterizerState;        // current rasterizer state binding
        DepthStencilStateInfo const * DepthStencilState;      // current depth-stencil state binding
    } Binding;

    unsigned int              BufferBinding[ 2 ];

    COLOR_CLAMP               ColorClamp;

    BlendingStateInfo         BlendState;             // current blend state
    float                     BlendColor[4];
    unsigned int              SampleMask[4];
    bool                      bSampleMaskEnabled;
    bool                      bLogicOpEnabled;

    RasterizerStateInfo       RasterizerState;       // current rasterizer state
    bool                      bPolygonOffsetEnabled;

    DepthStencilStateInfo     DepthStencilState;     // current depth-stencil state
    unsigned int              StencilRef;

    RenderPass const *        CurrentRenderPass;
    Rect2D                    CurrentRenderPassRenderArea;

    Rect2D                    CurrentScissor;

    bool                      bPrimitiveRestartEnabled;

    int                       SwapChainWidth;
    int                       SwapChainHeight;

    THash<>                   VAOHash;
    TPodArray< struct VertexArrayObject * > VAOCache;

    unsigned int              TotalPipelines;
    unsigned int              TotalRenderPasses;
    unsigned int              TotalFramebuffers;
    unsigned int              TotalTransformFeedbacks;
    unsigned int              TotalQueryPools;

    State *                   Next;
    State *                   Prev;

    static State *            StateHead;
    static State *            StateTail;
};

void SetCurrentState( State * _State );
State * GetCurrentState();

/*

Example:

//
// Declare current state
//

thread_local FState * CurrentState; // thread_local for multithreaded apps

//
// Implement SetCurrentState and GetCurrentState:
//

void SetCurrentState( FState * _State ) {
    CurrentState = _State;
}

FState * GetCurrentState() {
    return CurrentState;
}

//
// Usage (create):
//

CreateWindow(...)
wglMakeContextCurrent(...);

// Device can be shared for different states (OpenGL contexts must be shared too in this case)
Device.Initialize(...);

// Set current state (call this always after wglMakeContextCurrent)
SetCurrentState( &MyState );

// Initialize state
MyState.Initialize( &Device );

//
// Usage (destroy):
//

// After all objects destroyed
MyState.Deinitialize();
Device.Deinitialize();


Objects:
    Device
    State
    Buffer
    Texture
    Sampler
    ShaderModule
    Framebuffer
    Pipeline
    CommandBuffer
    RenderPass


This objects can be shared:
    Buffer, Texture, Sampler, ShaderModule


*/

}
