/*

Graphics Hardware Interface (GHI) is part of Angie Engine Source Code

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

#include "GHIRenderPass.h"
#include "GHIDevice.h"
#include "GHIState.h"
#include "GHIFramebuffer.h"
#include "GHITexture.h"
#include "LUT.h"

#include <assert.h>
#include <memory.h>
#include "GL/glew.h"

namespace GHI {

static int RenderPassIdGenerator = 0;

RenderPass::RenderPass() {
    pDevice = nullptr;
    Handle = nullptr;
}

RenderPass::~RenderPass() {
    Deinitialize();
}

void RenderPass::Initialize( RenderPassCreateInfo const & _CreateInfo ) {
    State * state = GetCurrentState();

    Deinitialize();

    assert( _CreateInfo.NumColorAttachments <= MAX_COLOR_ATTACHMENTS );
    assert( _CreateInfo.NumSubpasses <= MAX_SUBPASS_COUNT );

    Handle = ( void * )( size_t )++RenderPassIdGenerator;

    NumColorAttachments = _CreateInfo.NumColorAttachments;
    memcpy( ColorAttachments, _CreateInfo.pColorAttachments, sizeof( ColorAttachments[0] ) * NumColorAttachments );

    bHasDepthStencilAttachment = ( _CreateInfo.pDepthStencilAttachment != nullptr );
    if ( bHasDepthStencilAttachment ) {
        memcpy( &DepthStencilAttachment, _CreateInfo.pDepthStencilAttachment, sizeof( DepthStencilAttachment ) );
    }

    NumSubpasses = _CreateInfo.NumSubpasses;
    RenderSubpass * dst = Subpasses;
    for ( SubpassInfo * subpassDesc = _CreateInfo.pSubpasses; subpassDesc < &_CreateInfo.pSubpasses[_CreateInfo.NumSubpasses] ; subpassDesc++, dst++ ) {

        assert( subpassDesc->NumColorAttachments <= MAX_COLOR_ATTACHMENTS );

        dst->NumColorAttachments = subpassDesc->NumColorAttachments;

        for ( uint32_t j = 0 ; j < subpassDesc->NumColorAttachments ; j++ ) {
            dst->ColorAttachmentRefs[j] = subpassDesc->pColorAttachmentRefs[j];
        }
    }

    pDevice = state->GetDevice();

    state->TotalRenderPasses++;
}

void RenderPass::Deinitialize() {
    if ( !Handle ) {
        return;
    }

    State * state = GetCurrentState();

    if ( state->CurrentRenderPass == this ) {
        LogPrintf( "RenderPass::Deinitialize: destroying render pass without EndRenderPass()\n" );
        state->CurrentRenderPass = nullptr;
    }

    state->TotalRenderPasses--;

    pDevice = nullptr;
    Handle = nullptr;
}

}
