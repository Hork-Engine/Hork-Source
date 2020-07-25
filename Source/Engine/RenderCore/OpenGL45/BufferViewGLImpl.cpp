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

#include "BufferViewGLImpl.h"
#include "DeviceGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

#include <Core/Public/Logger.h>

namespace RenderCore {

ABufferViewGLImpl::ABufferViewGLImpl( ADeviceGLImpl * Device, SBufferViewCreateInfo const & CreateInfo, ABufferGLImpl * pBuffer )
    : pDevice( Device ), pSrcBuffer( pBuffer )
{
    pSrcBuffer->AddRef();

    GLuint bufferId = GL_HANDLE( pSrcBuffer->GetHandle() );
    if ( !bufferId ) {
        GLogger.Printf( "ABufferViewGLImpl::ctor: invalid buffer handle\n" );
        return;
    }

    bool bViewRange = CreateInfo.SizeInBytes > 0;

    size_t sizeInBytes = bViewRange ? CreateInfo.SizeInBytes : pSrcBuffer->GetSizeInBytes();
    size_t offset = bViewRange ? CreateInfo.Offset : 0;

    if ( !IsAligned( offset, pDevice->DeviceCaps[DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT] ) ) {
        GLogger.Printf( "ABufferViewGLImpl::ctor: buffer offset is not aligned\n" );
        return;
    }

    if ( offset + sizeInBytes > pSrcBuffer->GetSizeInBytes() ) {
        GLogger.Printf( "ABufferViewGLImpl::ctor: invalid buffer range\n" );
        return;
    }

    if ( sizeInBytes > pDevice->DeviceCaps[DEVICE_CAPS_BUFFER_VIEW_MAX_SIZE] ) {
        GLogger.Printf( "ABufferViewGLImpl::ctor: buffer view size > BUFFER_VIEW_MAX_SIZE\n" );
        return;
    }

    GLuint textureId;
    glCreateTextures( GL_TEXTURE_BUFFER, 1, &textureId );

    TableInternalPixelFormat const * format = &InternalFormatLUT[ CreateInfo.Format ];

    if ( bViewRange ) {
        glTextureBufferRange( textureId, format->InternalFormat, bufferId, offset, sizeInBytes );
    } else {
        glTextureBuffer( textureId, format->InternalFormat, bufferId );
    }

    Handle = ( void * )( size_t )textureId;

    // TODO:
    //pDevice->TotalBufferViews++;
}

ABufferViewGLImpl::~ABufferViewGLImpl()
{
    if ( Handle ) {
        GLuint id = GL_HANDLE( Handle );
        glDeleteTextures( 1, &id );
        //pDevice->TotalBufferViews--;
    }

    pSrcBuffer->RemoveRef();
}


}
