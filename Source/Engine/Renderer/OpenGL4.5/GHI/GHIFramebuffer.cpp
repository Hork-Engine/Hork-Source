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

#include "GHIFramebuffer.h"
#include "GHIDevice.h"
#include "GHIState.h"
#include "GHITexture.h"
#include "LUT.h"

#include <assert.h>
#include <memory.h>
#include <GL/glew.h>

namespace GHI {

Framebuffer::Framebuffer() {
    pDevice = nullptr;
    Handle = nullptr;
}

Framebuffer::~Framebuffer() {
    Deinitialize();
}

void Framebuffer::Initialize( FramebufferCreateInfo const & _CreateInfo ) {
    State * state = GetCurrentState();

    GLuint framebufferId;

    Deinitialize();

    assert( _CreateInfo.NumColorAttachments <= MAX_COLOR_ATTACHMENTS );

    glCreateFramebuffers( 1, &framebufferId );
    if ( !glIsFramebuffer( framebufferId ) ) {
        LogPrintf( "Framebuffer::Initialize: couldn't create framebuffer\n" );
        return;
    }
    glNamedFramebufferParameteri( framebufferId, GL_FRAMEBUFFER_DEFAULT_WIDTH, _CreateInfo.Width );
    glNamedFramebufferParameteri( framebufferId, GL_FRAMEBUFFER_DEFAULT_HEIGHT, _CreateInfo.Height );
    //glNamedFramebufferParameteri( framebufferId, GL_FRAMEBUFFER_DEFAULT_LAYERS, _CreateInfo.Layers );
    //todo: glNamedFramebufferParameteri( framebufferId, GL_FRAMEBUFFER_DEFAULT_SAMPLES, _DefaultSamplesCount );
    //todo: glNamedFramebufferParameteri( framebufferId, GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS, _DefaultFixedSampleLocations );

    // TODO: Check?
    //    GL_MAX_FRAMEBUFFER_WIDTH
    //    GL_MAX_FRAMEBUFFER_HEIGHT
    //    GL_MAX_FRAMEBUFFER_LAYERS

    glNamedFramebufferDrawBuffer( framebufferId, GL_NONE );

    for ( int i = 0 ; i < _CreateInfo.NumColorAttachments ; i++ ) {
        FramebufferAttachmentInfo const * attachment = &_CreateInfo.pColorAttachments[ i ];
        Texture const * texture = attachment->pTexture;
        GLuint textureId = GL_HANDLE( texture->GetHandle() );
        GLenum attachmentName = GL_COLOR_ATTACHMENT0 + i;

        if ( _CreateInfo.Width != ( texture->GetWidth() >> attachment->LodNum )
            || _CreateInfo.Height != ( texture->GetHeight() >> attachment->LodNum ) ) {
            LogPrintf( "Framebuffer::Initialize: invalid texture resolution\n" );
        }

        if ( texture->IsTextureBuffer() ) {
            LogPrintf( "Framebuffer::Initialize: texture buffers cannot be attached to framebuffer\n" );
            continue;
        }

        if ( attachment->bLayered ) {
            glNamedFramebufferTextureLayer( framebufferId, attachmentName, textureId, attachment->LodNum, attachment->LayerNum );
        } else {
            glNamedFramebufferTexture( framebufferId, attachmentName, textureId, attachment->LodNum );
        }
    }

    Handle = ( void * )( size_t )framebufferId;

    NumColorAttachments = _CreateInfo.NumColorAttachments;
    memcpy( ColorAttachments, _CreateInfo.pColorAttachments, sizeof( ColorAttachments[0] ) * NumColorAttachments );

    bHasDepthStencilAttachment = ( _CreateInfo.pDepthStencilAttachment != nullptr );
    if ( bHasDepthStencilAttachment ) {
        memcpy( &DepthStencilAttachment, _CreateInfo.pDepthStencilAttachment, sizeof( DepthStencilAttachment ) );

        FramebufferAttachmentInfo const * attachment = _CreateInfo.pDepthStencilAttachment;
        Texture const * texture = attachment->pTexture;
        GLuint textureId = GL_HANDLE( texture->GetHandle() );
        GLenum attachmentName;

        do {

            if ( texture->IsTextureBuffer() ) {
                LogPrintf( "Framebuffer::Initialize: texture buffers cannot be attached to framebuffer\n" );
                break;
            }

            // TODO: table
            switch ( texture->GetInternalPixelFormat() ) {
            case INTERNAL_PIXEL_FORMAT_STENCIL1:
            case INTERNAL_PIXEL_FORMAT_STENCIL4:
            case INTERNAL_PIXEL_FORMAT_STENCIL8:
            case INTERNAL_PIXEL_FORMAT_STENCIL16:
                attachmentName = GL_STENCIL_ATTACHMENT;
                break;
            case INTERNAL_PIXEL_FORMAT_DEPTH16:
            case INTERNAL_PIXEL_FORMAT_DEPTH24:
            case INTERNAL_PIXEL_FORMAT_DEPTH32:
                attachmentName = GL_DEPTH_ATTACHMENT;
                break;
            case INTERNAL_PIXEL_FORMAT_DEPTH24_STENCIL8:
            case INTERNAL_PIXEL_FORMAT_DEPTH32F_STENCIL8:
                attachmentName = GL_DEPTH_STENCIL_ATTACHMENT;
                break;
            default:
                assert( 0 );
                attachmentName = GL_DEPTH_STENCIL_ATTACHMENT;
            }

            if ( attachment->bLayered ) {
                glNamedFramebufferTextureLayer( framebufferId, attachmentName, textureId, 0, attachment->LayerNum );
            } else {
                glNamedFramebufferTexture( framebufferId, attachmentName, textureId, 0 );
            }

        } while (0);
    }

    //GLenum framebufferStatus = glCheckNamedFramebufferStatus( framebufferId, GL_DRAW_FRAMEBUFFER );

    Width = _CreateInfo.Width;
    Height = _CreateInfo.Height;

    state->TotalFramebuffers++;

    pDevice = state->GetDevice();

    // NOTE: текущие параметры буфера можно получить с помощью следующих функций:
    // glGetBufferParameteri64v        glGetNamedBufferParameteri64v
    // glGetBufferParameteriv          glGetNamedBufferParameteriv
}

void Framebuffer::Deinitialize() {
    if ( !Handle ) {
        return;
    }

    State * state = GetCurrentState();

    GLuint framebufferId = GL_HANDLE( Handle );

    glDeleteFramebuffers( 1, &framebufferId );
    if ( state->Binding.DrawFramebuffer == framebufferId ) {
        state->Binding.DrawFramebuffer = (unsigned int)~0;
    }
    if ( state->Binding.ReadFramebuffer == framebufferId ) {
        state->Binding.ReadFramebuffer = (unsigned int)~0;
    }

    state->TotalFramebuffers--;

    pDevice = nullptr;
    Handle = nullptr;
}

bool Framebuffer::ChooseReadBuffer( FRAMEBUFFER_ATTACHMENT _Attachment ) {

    GLuint framebufferId = GL_HANDLE( Handle );
    bool isDefaultFramebuffer = false;//IsDefaultFramebuffer( framebuffer );

    if ( _Attachment < FB_DEPTH_ATTACHMENT ) {

        if ( isDefaultFramebuffer ) {
            return false;
        }

        // TODO: check _Attachment < MAX_COLOR_ATTACHMENTS

        glNamedFramebufferReadBuffer( framebufferId, GL_COLOR_ATTACHMENT0 + _Attachment );
    } else if ( _Attachment <= FB_DEPTH_STENCIL_ATTACHMENT ) {

        if ( isDefaultFramebuffer ) {
            return false;
        }

        // Depth and Stencil read directly from framebuffer
        // There is no need to select read buffer

    } else {

        if ( !isDefaultFramebuffer ) {
            return false;
        }

        glNamedFramebufferReadBuffer( 0, FramebufferAttachmentLUT[ _Attachment - FB_DEPTH_ATTACHMENT ] );
    }

    return true;
}

void Framebuffer::BindReadFramebuffer() {
    GLuint framebufferId = GL_HANDLE( Handle );

    State * state = GetCurrentState();

    if ( state->Binding.ReadFramebuffer != framebufferId ) {
        glBindFramebuffer( GL_READ_FRAMEBUFFER, framebufferId );
        state->Binding.ReadFramebuffer = framebufferId;
    }
}

bool Framebuffer::Read( FRAMEBUFFER_ATTACHMENT _Attachment,
                        Rect2D const & _SrcRect,
                        FRAMEBUFFER_CHANNEL _FramebufferChannel,
                        FRAMEBUFFER_OUTPUT _FramebufferOutput,
                        COLOR_CLAMP _ColorClamp,
                        size_t _SizeInBytes,
                        unsigned int _Alignment,               // Specifies alignment of destination data
                        void * _SysMem ) {
    State * state = GetCurrentState();

    if ( !ChooseReadBuffer( _Attachment ) ) {
        LogPrintf( "Framebuffer::Read: invalid framebuffer attachment\n" );
        return false;
    }

    state->PackAlignment( _Alignment );

    BindReadFramebuffer();

    state->ClampReadColor( _ColorClamp );

    glReadnPixels( _SrcRect.X,
                   _SrcRect.Y,
                   _SrcRect.Width,
                   _SrcRect.Height,
                   FramebufferChannelLUT[ _FramebufferChannel ],
                   FramebufferOutputLUT[ _FramebufferOutput ],
                   (GLsizei)_SizeInBytes,
                   _SysMem );
    return true;
}

bool Framebuffer::Invalidate( uint16_t _NumFramebufferAttachments, FRAMEBUFFER_ATTACHMENT const * _Attachments ) {

    static_assert( sizeof( GLenum ) == sizeof( State::TmpHandles[ 0 ] ), "Framebuffer::Invalidate" );

    if ( _NumFramebufferAttachments == 0 ) {
        return true;
    }

    if ( !_Attachments ) {
        return false;
    }

    State * state = GetCurrentState();

    for ( uint16_t i = 0 ; i < _NumFramebufferAttachments ; i++ ) {
        if ( _Attachments[ i ] < FB_DEPTH_ATTACHMENT ) {
            // TODO: check _Attachments[ i ] < MAX_COLOR_ATTACHMENTS

            state->TmpHandles[ i ] = GL_COLOR_ATTACHMENT0 + _Attachments[ i ];
        } else {
            state->TmpHandles[ i ] = FramebufferAttachmentLUT[ _Attachments[ i ] - FB_DEPTH_ATTACHMENT ];
        }
    }

    glInvalidateNamedFramebufferData( GL_HANDLE( Handle ), _NumFramebufferAttachments, state->TmpHandles );

    return true;
}

bool Framebuffer::InvalidateRect( uint16_t _NumFramebufferAttachments,
                                  FRAMEBUFFER_ATTACHMENT const * _Attachments,
                                  Rect2D const & _Rect ) {

    static_assert( sizeof( GLenum ) == sizeof( State::TmpHandles[ 0 ] ), "Framebuffer::InvalidateRect" );

    if ( _NumFramebufferAttachments == 0 ) {
        return true;
    }

    if ( !_Attachments ) {
        return false;
    }

    State * state = GetCurrentState();

    for ( uint16_t i = 0 ; i < _NumFramebufferAttachments ; i++ ) {
        if ( _Attachments[ i ] < FB_DEPTH_ATTACHMENT ) {
            // TODO: check _Attachments[ i ] < MAX_COLOR_ATTACHMENTS

            state->TmpHandles[ i ] = GL_COLOR_ATTACHMENT0 + _Attachments[ i ];
        } else {
            state->TmpHandles[ i ] = FramebufferAttachmentLUT[ _Attachments[ i ] - FB_DEPTH_ATTACHMENT ];
        }
    }

    glInvalidateNamedFramebufferSubData( GL_HANDLE( Handle ),
                                         _NumFramebufferAttachments,
                                         state->TmpHandles,
                                         _Rect.X,
                                         _Rect.Y,
                                         _Rect.Width,
                                         _Rect.Height );

    return true;
}

}
