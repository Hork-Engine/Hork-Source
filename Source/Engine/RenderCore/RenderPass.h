/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "DeviceObject.h"
#include "Framebuffer.h"

namespace RenderCore {

struct SAttachmentRef
{
    uint32_t Attachment;

    SAttachmentRef() {}
    SAttachmentRef( uint32_t _Attachment ) : Attachment( _Attachment ) {}
};

struct SSubpassInfo
{
    uint32_t               NumColorAttachments;
    SAttachmentRef const * pColorAttachmentRefs;
};

enum ATTACHMENT_LOAD_OP : uint8_t
{
    ATTACHMENT_LOAD_OP_LOAD = 0,
    ATTACHMENT_LOAD_OP_CLEAR = 1,
    ATTACHMENT_LOAD_OP_DONT_CARE = 2,
};

struct SAttachmentInfo
{
    ATTACHMENT_LOAD_OP LoadOp;

    SAttachmentInfo()
        : LoadOp( ATTACHMENT_LOAD_OP_LOAD )
    {
    }

    SAttachmentInfo & SetLoadOp( ATTACHMENT_LOAD_OP Op ) {
        LoadOp = Op;
        return *this;
    }
};

struct SRenderPassCreateInfo
{
    int               NumColorAttachments;
    SAttachmentInfo * pColorAttachments;

    SAttachmentInfo * pDepthStencilAttachment;

    int               NumSubpasses;
    SSubpassInfo *    pSubpasses;

    //uint32_t        NumDependencies;
    //SubpassDependency const * pDependencies;
};

struct SRenderSubpass
{
    uint32_t          NumColorAttachments;
    SAttachmentRef    ColorAttachmentRefs[ MAX_COLOR_ATTACHMENTS ];
};

typedef union SClearColorValue
{
    float       Float32[4];
    int32_t     Int32[4];
    uint32_t    UInt32[4];
} SClearColorValue;

inline SClearColorValue MakeClearColorValue( float R, float G, float B, float A ) {
    SClearColorValue v;
    v.Float32[0] = R;
    v.Float32[1] = G;
    v.Float32[2] = B;
    v.Float32[3] = A;
    return v;
}

inline SClearColorValue MakeClearColorValue( int32_t R, int32_t G, int32_t B, int32_t A ) {
    SClearColorValue v;
    v.Int32[0] = R;
    v.Int32[1] = G;
    v.Int32[2] = B;
    v.Int32[3] = A;
    return v;
}

inline SClearColorValue MakeClearColorValue( uint32_t R, uint32_t G, uint32_t B, uint32_t A ) {
    SClearColorValue v;
    v.UInt32[0] = R;
    v.UInt32[1] = G;
    v.UInt32[2] = B;
    v.UInt32[3] = A;
    return v;
}

typedef struct SClearDepthStencilValue
{
    float       Depth;
    uint32_t    Stencil;
} SClearDepthStencilValue;

inline SClearDepthStencilValue MakeClearDepthStencilValue( float Depth, uint32_t Stencil ) {
    SClearDepthStencilValue v;
    v.Depth = Depth;
    v.Stencil = Stencil;
    return v;
}

class IRenderPass : public IDeviceObject
{
public:
    IRenderPass( IDevice * Device ) : IDeviceObject( Device ) {}
};

}
