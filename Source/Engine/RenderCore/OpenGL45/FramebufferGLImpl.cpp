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

#include "FramebufferGLImpl.h"
#include "DeviceGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "TextureGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

#include <Core/Public/Logger.h>

namespace RenderCore {

AFramebufferGLImpl::AFramebufferGLImpl( ADeviceGLImpl * _Device, SFramebufferCreateInfo const & _CreateInfo, bool _Default )
    : IFramebuffer( _Device ), pDevice( _Device )
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
        ITexture const * texture = attachment->pTexture;
        GLuint textureId = texture->GetHandleNativeGL();
        GLenum attachmentName = GL_COLOR_ATTACHMENT0 + i;

        if ( _CreateInfo.Width != ( texture->GetWidth() >> attachment->LodNum )
            || _CreateInfo.Height != ( texture->GetHeight() >> attachment->LodNum ) ) {
            GLogger.Printf( "Framebuffer::Initialize: invalid texture resolution\n" );
        }

        if ( attachment->Type == ATTACH_LAYER ) {
            glNamedFramebufferTextureLayer( framebufferId, attachmentName, textureId, attachment->LodNum, attachment->LayerNum );
        }
        else {
            glNamedFramebufferTexture( framebufferId, attachmentName, textureId, attachment->LodNum );
        }

        Textures[i] = attachment->pTexture;
    }

    SetHandleNativeGL( framebufferId );

    NumColorAttachments = _CreateInfo.NumColorAttachments;
    memcpy( ColorAttachments, _CreateInfo.pColorAttachments, sizeof( ColorAttachments[0] ) * NumColorAttachments );

    bHasDepthStencilAttachment = ( _CreateInfo.pDepthStencilAttachment != nullptr );
    if ( bHasDepthStencilAttachment ) {
        memcpy( &DepthStencilAttachment, _CreateInfo.pDepthStencilAttachment, sizeof( DepthStencilAttachment ) );

        SFramebufferAttachmentInfo const * attachment = _CreateInfo.pDepthStencilAttachment;
        ITexture const * texture = attachment->pTexture;
        GLuint textureId = texture->GetHandleNativeGL();
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
        }
        else {
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

AFramebufferGLImpl::~AFramebufferGLImpl()
{
    if ( bDefault ) {
        return;
    }

    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    GLuint framebufferId = GetHandleNativeGL();

    if ( framebufferId ) {
        ctx->UnbindFramebuffer( this );

        glDeleteFramebuffers( 1, &framebufferId );
    }

    pDevice->TotalFramebuffers--;
}

bool IFramebuffer::IsAttachmentsOutdated() const
{
    for ( int i = 0 ; i < NumColorAttachments ; i++ ) {
        if ( Textures[i].IsExpired() ) {
            return true;
        }
    }

    return bHasDepthStencilAttachment && DepthAttachment.IsExpired();
}

bool AFramebufferGLImpl::ChooseReadBuffer( FRAMEBUFFER_ATTACHMENT _Attachment ) const
{
    GLuint framebufferId = GetHandleNativeGL();

    if ( _Attachment < FB_DEPTH_ATTACHMENT ) {
        if ( bDefault ) {
            return false;
        }

        // TODO: check _Attachment < MAX_COLOR_ATTACHMENTS

        glNamedFramebufferReadBuffer( framebufferId, GL_COLOR_ATTACHMENT0 + _Attachment );
    }
    else if ( _Attachment <= FB_DEPTH_STENCIL_ATTACHMENT ) {
        if ( bDefault ) {
            return false;
        }

        // Depth and Stencil read directly from framebuffer
        // There is no need to select read buffer
    }
    else {
        if ( !bDefault ) {
            return false;
        }
        glNamedFramebufferReadBuffer( 0, FramebufferAttachmentLUT[ _Attachment - FB_DEPTH_ATTACHMENT ] );
    }

    return true;
}

bool AFramebufferGLImpl::Read( FRAMEBUFFER_ATTACHMENT _Attachment,
                               SRect2D const & _SrcRect,
                               FRAMEBUFFER_CHANNEL _FramebufferChannel,
                               FRAMEBUFFER_OUTPUT _FramebufferOutput,
                               COLOR_CLAMP _ColorClamp,
                               size_t _SizeInBytes,
                               unsigned int _Alignment,               // Specifies alignment of destination data
                               void * _SysMem )
{
    AImmediateContextGLImpl * ctx = AImmediateContextGLImpl::GetCurrent();

    if ( !ChooseReadBuffer( _Attachment ) ) {
        GLogger.Printf( "Framebuffer::Read: invalid framebuffer attachment\n" );
        return false;
    }

    ctx->PackAlignment( _Alignment );

    ctx->BindReadFramebuffer( this );

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

bool AFramebufferGLImpl::Invalidate( uint16_t _NumFramebufferAttachments, FRAMEBUFFER_ATTACHMENT const * _Attachments )
{
    if ( _NumFramebufferAttachments == 0 ) {
        return true;
    }

    if ( !_Attachments ) {
        return false;
    }

    GLenum * handles = (GLenum *)StackAlloc( sizeof( GLenum ) * _NumFramebufferAttachments );

    for ( uint16_t i = 0 ; i < _NumFramebufferAttachments ; i++ ) {
        if ( _Attachments[ i ] < FB_DEPTH_ATTACHMENT ) {
            // TODO: check _Attachments[ i ] < MAX_COLOR_ATTACHMENTS

            handles[ i ] = GL_COLOR_ATTACHMENT0 + _Attachments[ i ];
        }
        else {
            handles[ i ] = FramebufferAttachmentLUT[ _Attachments[ i ] - FB_DEPTH_ATTACHMENT ];
        }
    }

    glInvalidateNamedFramebufferData( GetHandleNativeGL(), _NumFramebufferAttachments, handles );

    return true;
}

bool AFramebufferGLImpl::InvalidateRect( uint16_t _NumFramebufferAttachments,
                                  FRAMEBUFFER_ATTACHMENT const * _Attachments,
                                  SRect2D const & _Rect )
{
    if ( _NumFramebufferAttachments == 0 ) {
        return true;
    }

    if ( !_Attachments ) {
        return false;
    }

    GLenum * handles = (GLenum *)StackAlloc( sizeof( GLenum ) * _NumFramebufferAttachments );

    for ( uint16_t i = 0 ; i < _NumFramebufferAttachments ; i++ ) {
        if ( _Attachments[ i ] < FB_DEPTH_ATTACHMENT ) {
            // TODO: check _Attachments[ i ] < MAX_COLOR_ATTACHMENTS

            handles[ i ] = GL_COLOR_ATTACHMENT0 + _Attachments[ i ];
        }
        else {
            handles[ i ] = FramebufferAttachmentLUT[ _Attachments[ i ] - FB_DEPTH_ATTACHMENT ];
        }
    }

    glInvalidateNamedFramebufferSubData( GetHandleNativeGL(),
                                         _NumFramebufferAttachments,
                                         handles,
                                         _Rect.X,
                                         _Rect.Y,
                                         _Rect.Width,
                                         _Rect.Height );

    return true;
}

}
