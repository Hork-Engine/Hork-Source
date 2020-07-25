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

#include "FramebufferGLImpl.h"
#include "DeviceGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "TextureGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

#include <Core/Public/Logger.h>

namespace RenderCore {

AFramebufferGLImpl::AFramebufferGLImpl( ADeviceGLImpl * _Device, SFramebufferCreateInfo const & _CreateInfo, bool _Default )
    : pDevice( _Device )
{
    Width = 0;
    Height = 0;
    NumColorAttachments = 0;
    bHasDepthStencilAttachment = 0;
    bDefault = _Default;

    if ( bDefault ) {
        return;
    }

    GLuint framebufferId;

    AN_ASSERT( _CreateInfo.NumColorAttachments <= MAX_COLOR_ATTACHMENTS );

    // TODO: Create framebuffer for each context

    glCreateFramebuffers( 1, &framebufferId );

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
        SFramebufferAttachmentInfo const * attachment = &_CreateInfo.pColorAttachments[ i ];
        ATextureGLImpl const * texture = static_cast< ATextureGLImpl const * >( attachment->pTexture );
        GLuint textureId = GL_HANDLE( texture->GetHandle() );
        GLenum attachmentName = GL_COLOR_ATTACHMENT0 + i;

        if ( _CreateInfo.Width != ( texture->GetWidth() >> attachment->LodNum )
            || _CreateInfo.Height != ( texture->GetHeight() >> attachment->LodNum ) ) {
            GLogger.Printf( "Framebuffer::Initialize: invalid texture resolution\n" );
        }

        if ( attachment->Type == ATTACH_LAYER ) {
            glNamedFramebufferTextureLayer( framebufferId, attachmentName, textureId, attachment->LodNum, attachment->LayerNum );
        } else {
            glNamedFramebufferTexture( framebufferId, attachmentName, textureId, attachment->LodNum );
        }

        Textures[i] = attachment->pTexture;
    }

    Handle = ( void * )( size_t )framebufferId;

    NumColorAttachments = _CreateInfo.NumColorAttachments;
    memcpy( ColorAttachments, _CreateInfo.pColorAttachments, sizeof( ColorAttachments[0] ) * NumColorAttachments );

    bHasDepthStencilAttachment = ( _CreateInfo.pDepthStencilAttachment != nullptr );
    if ( bHasDepthStencilAttachment ) {
        memcpy( &DepthStencilAttachment, _CreateInfo.pDepthStencilAttachment, sizeof( DepthStencilAttachment ) );

        SFramebufferAttachmentInfo const * attachment = _CreateInfo.pDepthStencilAttachment;
        ATextureGLImpl const * texture = static_cast< ATextureGLImpl const * >( attachment->pTexture );
        GLuint textureId = GL_HANDLE( texture->GetHandle() );
        GLenum attachmentName;

        // TODO: table
        switch ( texture->GetFormat() ) {
        case TEXTURE_FORMAT_STENCIL1:
        case TEXTURE_FORMAT_STENCIL4:
        case TEXTURE_FORMAT_STENCIL8:
        case TEXTURE_FORMAT_STENCIL16:
            attachmentName = GL_STENCIL_ATTACHMENT;
            break;
        case TEXTURE_FORMAT_DEPTH16:
        case TEXTURE_FORMAT_DEPTH24:
        case TEXTURE_FORMAT_DEPTH32:
            attachmentName = GL_DEPTH_ATTACHMENT;
            break;
        case TEXTURE_FORMAT_DEPTH24_STENCIL8:
        case TEXTURE_FORMAT_DEPTH32F_STENCIL8:
            attachmentName = GL_DEPTH_STENCIL_ATTACHMENT;
            break;
        default:
            AN_ASSERT( 0 );
            attachmentName = GL_DEPTH_STENCIL_ATTACHMENT;
        }

        if ( attachment->Type == ATTACH_LAYER ) {
            glNamedFramebufferTextureLayer( framebufferId, attachmentName, textureId, 0, attachment->LayerNum );
        } else {
            glNamedFramebufferTexture( framebufferId, attachmentName, textureId, 0 );
        }

        DepthAttachment = attachment->pTexture;
    }

    //GLenum framebufferStatus = glCheckNamedFramebufferStatus( framebufferId, GL_DRAW_FRAMEBUFFER );

    Width = _CreateInfo.Width;
    Height = _CreateInfo.Height;

    pDevice->TotalFramebuffers++;

    // NOTE: текущие параметры буфера можно получить с помощью следующих функций:
    // glGetBufferParameteri64v        glGetNamedBufferParameteri64v
    // glGetBufferParameteriv          glGetNamedBufferParameteriv
}

AFramebufferGLImpl::~AFramebufferGLImpl() {
    if ( bDefault ) {
        return;
    }

    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    if ( Handle ) {
        GLuint framebufferId = GL_HANDLE( Handle );

        glDeleteFramebuffers( 1, &framebufferId );
        if ( ctx->Binding.DrawFramebuffer == framebufferId ) {
            ctx->Binding.DrawFramebuffer = (unsigned int)~0;
        }
        if ( ctx->Binding.ReadFramebuffer == framebufferId ) {
            ctx->Binding.ReadFramebuffer = (unsigned int)~0;
        }
    }

    pDevice->TotalFramebuffers--;
}

bool IFramebuffer::IsAttachmentsOutdated() const {
    for ( int i = 0 ; i < NumColorAttachments ; i++ ) {
        if ( Textures[i].IsExpired() ) {
            return true;
        }
    }

    return bHasDepthStencilAttachment && DepthAttachment.IsExpired();
}

bool AFramebufferGLImpl::ChooseReadBuffer( FRAMEBUFFER_ATTACHMENT _Attachment ) const {

    GLuint framebufferId = GL_HANDLE( Handle );

    if ( _Attachment < FB_DEPTH_ATTACHMENT ) {

        if ( bDefault ) {
            return false;
        }

        // TODO: check _Attachment < MAX_COLOR_ATTACHMENTS

        glNamedFramebufferReadBuffer( framebufferId, GL_COLOR_ATTACHMENT0 + _Attachment );
    } else if ( _Attachment <= FB_DEPTH_STENCIL_ATTACHMENT ) {

        if ( bDefault ) {
            return false;
        }

        // Depth and Stencil read directly from framebuffer
        // There is no need to select read buffer

    } else {

        if ( !bDefault ) {
            return false;
        }

        glNamedFramebufferReadBuffer( 0, FramebufferAttachmentLUT[ _Attachment - FB_DEPTH_ATTACHMENT ] );
    }

    return true;
}

void AFramebufferGLImpl::BindReadFramebuffer() const {
    GLuint framebufferId = GL_HANDLE( Handle );

    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    if ( ctx->Binding.ReadFramebuffer != framebufferId ) {
        glBindFramebuffer( GL_READ_FRAMEBUFFER, framebufferId );
        ctx->Binding.ReadFramebuffer = framebufferId;
    }
}

bool AFramebufferGLImpl::Read( FRAMEBUFFER_ATTACHMENT _Attachment,
                               SRect2D const & _SrcRect,
                               FRAMEBUFFER_CHANNEL _FramebufferChannel,
                               FRAMEBUFFER_OUTPUT _FramebufferOutput,
                               COLOR_CLAMP _ColorClamp,
                               size_t _SizeInBytes,
                               unsigned int _Alignment,               // Specifies alignment of destination data
                               void * _SysMem ) {
    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    if ( !ChooseReadBuffer( _Attachment ) ) {
        GLogger.Printf( "Framebuffer::Read: invalid framebuffer attachment\n" );
        return false;
    }

    ctx->PackAlignment( _Alignment );

    BindReadFramebuffer();

    ctx->ClampReadColor( _ColorClamp );

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

bool AFramebufferGLImpl::Invalidate( uint16_t _NumFramebufferAttachments, FRAMEBUFFER_ATTACHMENT const * _Attachments ) {

    static_assert( sizeof( GLenum ) == sizeof( AImmediateContextGLImpl::TmpHandles[ 0 ] ), "Framebuffer::Invalidate" );

    if ( _NumFramebufferAttachments == 0 ) {
        return true;
    }

    if ( !_Attachments ) {
        return false;
    }

    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    for ( uint16_t i = 0 ; i < _NumFramebufferAttachments ; i++ ) {
        if ( _Attachments[ i ] < FB_DEPTH_ATTACHMENT ) {
            // TODO: check _Attachments[ i ] < MAX_COLOR_ATTACHMENTS

            ctx->TmpHandles[ i ] = GL_COLOR_ATTACHMENT0 + _Attachments[ i ];
        } else {
            ctx->TmpHandles[ i ] = FramebufferAttachmentLUT[ _Attachments[ i ] - FB_DEPTH_ATTACHMENT ];
        }
    }

    glInvalidateNamedFramebufferData( GL_HANDLE( Handle ), _NumFramebufferAttachments, ctx->TmpHandles );

    return true;
}

bool AFramebufferGLImpl::InvalidateRect( uint16_t _NumFramebufferAttachments,
                                  FRAMEBUFFER_ATTACHMENT const * _Attachments,
                                  SRect2D const & _Rect ) {

    static_assert( sizeof( GLenum ) == sizeof( AImmediateContextGLImpl::TmpHandles[ 0 ] ), "Framebuffer::InvalidateRect" );

    if ( _NumFramebufferAttachments == 0 ) {
        return true;
    }

    if ( !_Attachments ) {
        return false;
    }

    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    for ( uint16_t i = 0 ; i < _NumFramebufferAttachments ; i++ ) {
        if ( _Attachments[ i ] < FB_DEPTH_ATTACHMENT ) {
            // TODO: check _Attachments[ i ] < MAX_COLOR_ATTACHMENTS

            ctx->TmpHandles[ i ] = GL_COLOR_ATTACHMENT0 + _Attachments[ i ];
        } else {
            ctx->TmpHandles[ i ] = FramebufferAttachmentLUT[ _Attachments[ i ] - FB_DEPTH_ATTACHMENT ];
        }
    }

    glInvalidateNamedFramebufferSubData( GL_HANDLE( Handle ),
                                         _NumFramebufferAttachments,
                                         ctx->TmpHandles,
                                         _Rect.X,
                                         _Rect.Y,
                                         _Rect.Width,
                                         _Rect.Height );

    return true;
}

}
