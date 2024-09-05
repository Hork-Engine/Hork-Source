/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "BufferGLImpl.h"
#include "BufferViewGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "DeviceGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

HK_NAMESPACE_BEGIN

namespace RenderCore
{

static GLenum ChooseBufferUsageHint(MUTABLE_STORAGE_CLIENT_ACCESS _ClientAccess,
                                    MUTABLE_STORAGE_USAGE         _StorageUsage)
{
    switch (_StorageUsage)
    {
        case MUTABLE_STORAGE_STATIC:
            switch (_ClientAccess)
            {
                case MUTABLE_STORAGE_CLIENT_WRITE_ONLY: return GL_STATIC_DRAW;
                case MUTABLE_STORAGE_CLIENT_READ_ONLY: return GL_STATIC_READ;
                case MUTABLE_STORAGE_CLIENT_NO_TRANSFER: return GL_STATIC_COPY;
            }
            break;

        case MUTABLE_STORAGE_DYNAMIC:
            switch (_ClientAccess)
            {
                case MUTABLE_STORAGE_CLIENT_WRITE_ONLY: return GL_DYNAMIC_DRAW;
                case MUTABLE_STORAGE_CLIENT_READ_ONLY: return GL_DYNAMIC_READ;
                case MUTABLE_STORAGE_CLIENT_NO_TRANSFER: return GL_DYNAMIC_COPY;
            }
            break;

        case MUTABLE_STORAGE_STREAM:
            switch (_ClientAccess)
            {
                case MUTABLE_STORAGE_CLIENT_WRITE_ONLY: return GL_STREAM_DRAW;
                case MUTABLE_STORAGE_CLIENT_READ_ONLY: return GL_STREAM_READ;
                case MUTABLE_STORAGE_CLIENT_NO_TRANSFER: return GL_STREAM_COPY;
            }
            break;
    }

    return GL_STATIC_DRAW;
}

BufferGLImpl::BufferGLImpl(DeviceGLImpl* pDevice, BufferDesc const& Desc, const void* SysMem) :
    IBuffer(pDevice, Desc)
{
    GLuint id;
    GLint  size;

    glCreateBuffers(1, &id);

    // Allocate storage
    if (Desc.bImmutableStorage)
    {
        glNamedBufferStorage(id, Desc.SizeInBytes, SysMem, Desc.ImmutableStorageFlags); // 4.5 or GL_ARB_direct_state_access
        //glBufferStorage // 4.4 or GL_ARB_buffer_storage
    }
    else
    {
        glNamedBufferData(id, Desc.SizeInBytes, SysMem, ChooseBufferUsageHint(Desc.MutableClientAccess, Desc.MutableUsage)); // 4.5 or GL_ARB_direct_state_access
    }

    glGetNamedBufferParameteriv(id, GL_BUFFER_SIZE, &size);

    if (size != (GLint)Desc.SizeInBytes)
    {
        glDeleteBuffers(1, &id);

        LOG("BufferGLImpl::ctor: couldn't allocate buffer size {} bytes\n", Desc.SizeInBytes);
        return;
    }

    SetHandleNativeGL(id);

    pDevice->BufferMemoryAllocated += Desc.SizeInBytes;

    // NOTE: текущие параметры буфера можно получить с помощью следующих функций:
    // glGetBufferParameteri64v        glGetNamedBufferParameteri64v
    // glGetBufferParameteriv          glGetNamedBufferParameteriv
}

BufferGLImpl::~BufferGLImpl()
{
    GLuint id = GetHandleNativeGL();

    if (id)
    {
        glDeleteBuffers(1, &id);
    }

    static_cast<DeviceGLImpl*>(GetDevice())->BufferMemoryAllocated -= GetDesc().SizeInBytes;
}

bool BufferGLImpl::CreateView(BufferViewDesc const& BufferViewDesc, Ref<IBufferView>* ppBufferView)
{
    *ppBufferView = MakeRef<BufferViewGLImpl>(BufferViewDesc, this);
    return true;
}

bool BufferGLImpl::Orphan()
{
    if (GetDesc().bImmutableStorage)
    {
        LOG("Buffer::Orphan: expected mutable buffer\n");
        return false;
    }

    glNamedBufferData(GetHandleNativeGL(), GetDesc().SizeInBytes, nullptr, ChooseBufferUsageHint(GetDesc().MutableClientAccess, GetDesc().MutableUsage)); // 4.5

    return true;
}

void BufferGLImpl::Invalidate()
{
    glInvalidateBufferData(GetHandleNativeGL());
}

void BufferGLImpl::InvalidateRange(size_t _RangeOffset, size_t _RangeSize)
{
    glInvalidateBufferSubData(GetHandleNativeGL(), _RangeOffset, _RangeSize);
}

void BufferGLImpl::FlushMappedRange(size_t _RangeOffset, size_t _RangeSize)
{
    glFlushMappedNamedBufferRange(GetHandleNativeGL(), _RangeOffset, _RangeSize);
}

void BufferGLImpl::Read(void* _SysMem)
{
    ReadRange(0, GetDesc().SizeInBytes, _SysMem);
}

void BufferGLImpl::ReadRange(size_t _ByteOffset, size_t _SizeInBytes, void* _SysMem)
{
    ImmediateContextGLImpl::sGetCurrent()->ReadBufferRange(this, _ByteOffset, _SizeInBytes, _SysMem);
}

void BufferGLImpl::Write(const void* _SysMem)
{
    WriteRange(0, GetDesc().SizeInBytes, _SysMem);
}

void BufferGLImpl::WriteRange(size_t _ByteOffset, size_t _SizeInBytes, const void* _SysMem)
{
    ImmediateContextGLImpl::sGetCurrent()->WriteBufferRange(this, _ByteOffset, _SizeInBytes, _SysMem);
}

} // namespace RenderCore

HK_NAMESPACE_END
