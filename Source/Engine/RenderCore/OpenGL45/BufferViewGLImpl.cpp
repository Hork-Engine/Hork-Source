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

#include "BufferViewGLImpl.h"
#include "DeviceGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

namespace RenderCore
{

ABufferViewGLImpl::ABufferViewGLImpl(SBufferViewDesc const& Desc, ABufferGLImpl* pBuffer) :
    IBufferView(pBuffer->GetDevice(), Desc), pSrcBuffer(pBuffer)
{
    pSrcBuffer->AddRef();

    GLuint bufferId = pSrcBuffer->GetHandleNativeGL();
    if (!bufferId)
    {
        GLogger.Printf("ABufferViewGLImpl::ctor: invalid buffer handle\n");
        return;
    }

    bool bViewRange = Desc.SizeInBytes > 0;

    size_t sizeInBytes = bViewRange ? Desc.SizeInBytes : pSrcBuffer->GetSizeInBytes();
    size_t offset      = bViewRange ? Desc.Offset : 0;

    if (!IsAligned(offset, GetDevice()->GetDeviceCaps(DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT)))
    {
        GLogger.Printf("ABufferViewGLImpl::ctor: buffer offset is not aligned\n");
        return;
    }

    if (offset + sizeInBytes > pSrcBuffer->GetSizeInBytes())
    {
        GLogger.Printf("ABufferViewGLImpl::ctor: invalid buffer range\n");
        return;
    }

    if (sizeInBytes > GetDevice()->GetDeviceCaps(DEVICE_CAPS_BUFFER_VIEW_MAX_SIZE))
    {
        GLogger.Printf("ABufferViewGLImpl::ctor: buffer view size > BUFFER_VIEW_MAX_SIZE\n");
        return;
    }

    GLuint textureId;
    glCreateTextures(GL_TEXTURE_BUFFER, 1, &textureId);

    TableInternalPixelFormat const* format = &InternalFormatLUT[Desc.Format];

    InternalFormat = format->InternalFormat;

    if (offset == 0 && sizeInBytes == pSrcBuffer->GetSizeInBytes())
    {
        glTextureBuffer(textureId, InternalFormat, bufferId);
    }
    else
    {
        glTextureBufferRange(textureId, InternalFormat, bufferId, offset, sizeInBytes);
    }

    SetHandleNativeGL(textureId);
}

ABufferViewGLImpl::~ABufferViewGLImpl()
{
    GLuint id = GetHandleNativeGL();

    if (id)
    {
        glDeleteTextures(1, &id);
    }

    pSrcBuffer->RemoveRef();
}

void ABufferViewGLImpl::SetRange(size_t Offset, size_t SizeInBytes)
{
    if (!IsAligned(Offset, GetDevice()->GetDeviceCaps(DEVICE_CAPS_BUFFER_VIEW_OFFSET_ALIGNMENT)))
    {
        GLogger.Printf("ABufferViewGLImpl::SetRange: buffer offset is not aligned\n");
        return;
    }

    if (Offset + SizeInBytes > pSrcBuffer->GetSizeInBytes())
    {
        GLogger.Printf("ABufferViewGLImpl::SetRange: invalid buffer range\n");
        return;
    }

    if (SizeInBytes > GetDevice()->GetDeviceCaps(DEVICE_CAPS_BUFFER_VIEW_MAX_SIZE))
    {
        GLogger.Printf("ABufferViewGLImpl::SetRange: buffer view size > BUFFER_VIEW_MAX_SIZE\n");
        return;
    }

    glTextureBufferRange(GetHandleNativeGL(), InternalFormat, pSrcBuffer->GetHandleNativeGL(), Offset, SizeInBytes);

    Desc.Offset      = Offset;
    Desc.SizeInBytes = SizeInBytes;
}

size_t ABufferViewGLImpl::GetBufferOffset(uint16_t MipLevel) const
{
    int offset;
    glGetTextureLevelParameteriv(GetHandleNativeGL(), MipLevel, GL_TEXTURE_BUFFER_OFFSET, &offset);
    return offset;
}

size_t ABufferViewGLImpl::GetBufferSizeInBytes(uint16_t MipLevel) const
{
    int sizeInBytes;
    glGetTextureLevelParameteriv(GetHandleNativeGL(), MipLevel, GL_TEXTURE_BUFFER_SIZE, &sizeInBytes);
    return sizeInBytes;
}

} // namespace RenderCore
