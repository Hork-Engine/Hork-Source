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

#include "BufferGLImpl.h"
#include "DeviceGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

#include <Core/Public/Logger.h>

namespace RenderCore {

static GLenum ChooseBufferUsageHint( MUTABLE_STORAGE_CLIENT_ACCESS _ClientAccess,
                                     MUTABLE_STORAGE_USAGE _StorageUsage )
{
    switch ( _StorageUsage ) {
    case MUTABLE_STORAGE_STATIC:
        switch ( _ClientAccess ) {
        case MUTABLE_STORAGE_CLIENT_WRITE_ONLY: return GL_STATIC_DRAW;
        case MUTABLE_STORAGE_CLIENT_READ_ONLY: return GL_STATIC_READ;
        case MUTABLE_STORAGE_CLIENT_NO_TRANSFER: return GL_STATIC_COPY;
        }
        break;

    case MUTABLE_STORAGE_DYNAMIC:
        switch ( _ClientAccess ) {
        case MUTABLE_STORAGE_CLIENT_WRITE_ONLY: return GL_DYNAMIC_DRAW;
        case MUTABLE_STORAGE_CLIENT_READ_ONLY: return GL_DYNAMIC_READ;
        case MUTABLE_STORAGE_CLIENT_NO_TRANSFER: return GL_DYNAMIC_COPY;
        }
        break;

    case MUTABLE_STORAGE_STREAM:
        switch ( _ClientAccess ) {
        case MUTABLE_STORAGE_CLIENT_WRITE_ONLY: return GL_STREAM_DRAW;
        case MUTABLE_STORAGE_CLIENT_READ_ONLY: return GL_STREAM_READ;
        case MUTABLE_STORAGE_CLIENT_NO_TRANSFER: return GL_STREAM_COPY;
        }
        break;
    }

    return GL_STATIC_DRAW;
}

ABufferGLImpl::ABufferGLImpl( ADeviceGLImpl * _Device, SBufferCreateInfo const & _CreateInfo, const void * _SysMem )
    : IBuffer( _Device ), pDevice( _Device )
{
    GLuint id;
    GLint size;

    bImmutableStorage = _CreateInfo.bImmutableStorage;
    MutableClientAccess = _CreateInfo.MutableClientAccess;
    MutableUsage = _CreateInfo.MutableUsage;
    ImmutableStorageFlags = _CreateInfo.ImmutableStorageFlags;

    glCreateBuffers( 1, &id );

    // Allocate storage
    if ( _CreateInfo.bImmutableStorage ) {
        glNamedBufferStorage( id, _CreateInfo.SizeInBytes, _SysMem, _CreateInfo.ImmutableStorageFlags ); // 4.5 or GL_ARB_direct_state_access
        //glBufferStorage // 4.4 or GL_ARB_buffer_storage
    } else {
        glNamedBufferData( id, _CreateInfo.SizeInBytes, _SysMem, ChooseBufferUsageHint( _CreateInfo.MutableClientAccess, _CreateInfo.MutableUsage ) ); // 4.5 or GL_ARB_direct_state_access
    }

    glGetNamedBufferParameteriv( id, GL_BUFFER_SIZE, &size );

    SizeInBytes = size;

    if ( SizeInBytes != (GLint)_CreateInfo.SizeInBytes ) {
        glDeleteBuffers( 1, &id );

        GLogger.Printf( "ABufferGLImpl::ctor: couldn't allocate buffer size %u bytes\n", _CreateInfo.SizeInBytes );
        return;
    }

    SetHandleNativeGL( id );

    pDevice->TotalBuffers++;
    pDevice->BufferMemoryAllocated += SizeInBytes;

    // NOTE: текущие параметры буфера можно получить с помощью следующих функций:
    // glGetBufferParameteri64v        glGetNamedBufferParameteri64v
    // glGetBufferParameteriv          glGetNamedBufferParameteriv
}

ABufferGLImpl::~ABufferGLImpl()
{
    GLuint id = GetHandleNativeGL();

    if ( id ) {
        glDeleteBuffers( 1, &id );
    }

    pDevice->TotalBuffers--;
    pDevice->BufferMemoryAllocated -= SizeInBytes;
}

bool ABufferGLImpl::Realloc( size_t _SizeInBytes, const void * _SysMem )
{
    if ( bImmutableStorage ) {
        GLogger.Printf( "Buffer::Realloc: immutable buffer cannot be reallocated\n" );
        return false;
    }

    SizeInBytes = _SizeInBytes;

    glNamedBufferData( GetHandleNativeGL(), _SizeInBytes, _SysMem, ChooseBufferUsageHint( MutableClientAccess, MutableUsage ) ); // 4.5

    return true;
}

bool ABufferGLImpl::Orphan()
{
    if ( bImmutableStorage ) {
        GLogger.Printf( "Buffer::Orphan: expected mutable buffer\n" );
        return false;
    }

    glNamedBufferData( GetHandleNativeGL(), SizeInBytes, nullptr, ChooseBufferUsageHint( MutableClientAccess, MutableUsage ) ); // 4.5

    return true;
}

void ABufferGLImpl::Read( void * _SysMem )
{
    ReadRange( 0, SizeInBytes, _SysMem );
}

void ABufferGLImpl::ReadRange( size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem )
{
    glGetNamedBufferSubData( GetHandleNativeGL(), _ByteOffset, _SizeInBytes, _SysMem ); // 4.5 or GL_ARB_direct_state_access

    /*
    // Other path:
    GLint id = GetHandleNativeGL();
    GLenum target = BufferTargetLUT[ Type ].Target;
    GLint currentBinding;
    glGetIntegerv( BufferTargetLUT[ Type ].Binding, &currentBinding );
    if ( currentBinding == id ) {
        glBufferSubData( target, _ByteOffset, _SizeInBytes, _SysMem );
    } else {
        glBindBuffer( GL_COPY_READ_BUFFER, id );
        glGetBufferSubData( GL_COPY_READ_BUFFER, _ByteOffset, _SizeInBytes, _SysMem );
    }
    */
}

void ABufferGLImpl::Write( const void * _SysMem )
{
    WriteRange( 0, SizeInBytes, _SysMem );
}

void ABufferGLImpl::WriteRange( size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem )
{
    glNamedBufferSubData( GetHandleNativeGL(), _ByteOffset, _SizeInBytes, _SysMem ); // 4.5 or GL_ARB_direct_state_access

    /*
    // Other path:
    GLint id = GetHandleNativeGL();
    GLint currentBinding;
    GLenum target = BufferTargetLUT[ Type ].Target;
    glGetIntegerv( BufferTargetLUT[ Type ].Binding, &currentBinding );
    if ( currentBinding == id ) {
        glBufferSubData( target, _ByteOffset, _SizeInBytes, _SysMem );
    } else {
        glBindBuffer( target, id );
        glBufferSubData( target, _ByteOffset, _SizeInBytes, _SysMem );
        glBindBuffer( target, currentBinding );
    }
    */
    /*
    // Other path:
    GLint id = GetHandleNativeGL();
    GLenum target = BufferTargetLUT[ Type ].Target;
    GLint currentBinding;
    glGetIntegerv( BufferTargetLUT[ Type ].Binding, &currentBinding );
    if ( currentBinding == id ) {
        glBufferSubData( target, _ByteOffset, _SizeInBytes, _SysMem );
    } else {
        glBindBuffer( GL_COPY_WRITE_BUFFER, id );
        glBufferSubData( GL_COPY_WRITE_BUFFER, _ByteOffset, _SizeInBytes, _SysMem );
    }
    */
}

void * ABufferGLImpl::Map( MAP_TRANSFER _ClientServerTransfer,
                           MAP_INVALIDATE _Invalidate,
                           MAP_PERSISTENCE _Persistence,
                           bool _FlushExplicit,
                           bool _Unsynchronized )
{
    return MapRange( 0, SizeInBytes,
                     _ClientServerTransfer,
                     _Invalidate,
                     _Persistence,
                     _FlushExplicit,
                     _Unsynchronized );
}

void * ABufferGLImpl::MapRange( size_t _RangeOffset,
                                size_t _RangeSize,
                                MAP_TRANSFER _ClientServerTransfer,
                                MAP_INVALIDATE _Invalidate,
                                MAP_PERSISTENCE _Persistence,
                                bool _FlushExplicit,
                                bool _Unsynchronized )
{
    int flags = 0;

    switch ( _ClientServerTransfer ) {
    case MAP_TRANSFER_READ:
        flags |= GL_MAP_READ_BIT;
        break;
    case MAP_TRANSFER_WRITE:
        flags |= GL_MAP_WRITE_BIT;
        break;
    case MAP_TRANSFER_RW:
        flags |= GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        break;
    }

    if ( !flags ) {
        // At least on of the bits GL_MAP_READ_BIT or GL_MAP_WRITE_BIT
        // must be set
        GLogger.Printf( "Buffer::MapRange: invalid map transfer function\n" );
        return nullptr;
    }

    if ( _Invalidate != MAP_NO_INVALIDATE ) {
        if ( flags & GL_MAP_READ_BIT ) {
            // This flag may not be used in combination with GL_MAP_READ_BIT.
            GLogger.Printf( "Buffer::MapRange: MAP_NO_INVALIDATE may not be used in combination with MAP_TRANSFER_READ/MAP_TRANSFER_RW\n" );
            return nullptr;
        }

        if ( _Invalidate == MAP_INVALIDATE_ENTIRE_BUFFER ) {
            flags |= GL_MAP_INVALIDATE_BUFFER_BIT;
        } else if ( _Invalidate == MAP_INVALIDATE_RANGE ) {
            flags |= GL_MAP_INVALIDATE_RANGE_BIT;
        }
    }

    switch ( _Persistence ) {
    case MAP_NON_PERSISTENT:
        break;
    case MAP_PERSISTENT_COHERENT:
        flags |= GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        break;
    case MAP_PERSISTENT_NO_COHERENT:
        flags |= GL_MAP_PERSISTENT_BIT;
        break;
    }

    if ( _FlushExplicit ) {
        flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
    }

    if ( _Unsynchronized ) {
        flags |= GL_MAP_UNSYNCHRONIZED_BIT;
    }

    return glMapNamedBufferRange( GetHandleNativeGL(), _RangeOffset, _RangeSize, flags );
}

void ABufferGLImpl::Unmap()
{
    glUnmapNamedBuffer( GetHandleNativeGL() );
}

void * ABufferGLImpl::GetMapPointer()
{
    void * pointer = nullptr;
    glGetNamedBufferPointerv( GetHandleNativeGL(), GL_BUFFER_MAP_POINTER, &pointer );
    return pointer;
}

void ABufferGLImpl::Invalidate()
{
    glInvalidateBufferData( GetHandleNativeGL() );
}

void ABufferGLImpl::InvalidateRange( size_t _RangeOffset, size_t _RangeSize )
{
    glInvalidateBufferSubData( GetHandleNativeGL(), _RangeOffset, _RangeSize );
}

void ABufferGLImpl::FlushMappedRange( size_t _RangeOffset, size_t _RangeSize )
{
    glFlushMappedNamedBufferRange( GetHandleNativeGL(), _RangeOffset, _RangeSize );
}

}
