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

#include "GHIBuffer.h"
#include "GHIDevice.h"
#include "GHIState.h"
#include "LUT.h"

#include <GL/glew.h>

namespace GHI {

Buffer::Buffer() {
    pDevice = nullptr;
    Handle = nullptr;
}

Buffer::~Buffer() {
    Deinitialize();
}

static GLenum ChooseBufferUsageHint( MUTABLE_STORAGE_CLIENT_ACCESS _ClientAccess,
                                     MUTABLE_STORAGE_USAGE _StorageUsage ) {

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

void Buffer::Initialize( BufferCreateInfo const & _CreateInfo, const void * _SysMem ) {
    GLuint id;
    GLint size;

    Deinitialize();

    glCreateBuffers( 1, &id );

    if ( _CreateInfo.bImmutableStorage ) {
        glNamedBufferStorage( id, _CreateInfo.SizeInBytes, _SysMem, _CreateInfo.ImmutableStorageFlags ); // 4.5 or GL_ARB_direct_state_access
        //glBufferStorage // 4.4 or GL_ARB_buffer_storage
    } else {
        glNamedBufferData( id, _CreateInfo.SizeInBytes, _SysMem, ChooseBufferUsageHint( _CreateInfo.MutableClientAccess, _CreateInfo.MutableUsage ) ); // 4.5 or GL_ARB_direct_state_access
    }

    glGetNamedBufferParameteriv( id, GL_BUFFER_SIZE, &size );

    if ( size != (GLint)_CreateInfo.SizeInBytes ) {
        glDeleteBuffers( 1, &id );

        LogPrintf( "Buffer::Initialize: couldn't allocate buffer size %u bytes\n", _CreateInfo.SizeInBytes );
        return;
    }

    pDevice = GetCurrentState()->GetDevice();
    CreateInfo = _CreateInfo;
    Handle = ( void * )( size_t )id;
    UID = pDevice->GenerateUID();

    pDevice->TotalBuffers++;
    pDevice->BufferMemoryAllocated += CreateInfo.SizeInBytes;

    // NOTE: текущие параметры буфера можно получить с помощью следующих функций:
    // glGetBufferParameteri64v        glGetNamedBufferParameteri64v
    // glGetBufferParameteriv          glGetNamedBufferParameteriv
}

void Buffer::Deinitialize() {
    if ( !Handle ) {
        return;
    }

    GLuint id = GL_HANDLE( Handle );
    glDeleteBuffers( 1, &id );
    pDevice->TotalBuffers--;
    pDevice->BufferMemoryAllocated -= CreateInfo.SizeInBytes;

    pDevice = nullptr;
    Handle = nullptr;
}

bool Buffer::Realloc( size_t _NewByteLength, const void * _SysMem ) {
    if ( CreateInfo.bImmutableStorage ) {
        LogPrintf( "Buffer::Realloc: immutable buffer cannot be reallocated\n" );
        return false;
    }

    CreateInfo.SizeInBytes = _NewByteLength;

    glNamedBufferData( GL_HANDLE( Handle ), _NewByteLength, _SysMem, ChooseBufferUsageHint( CreateInfo.MutableClientAccess, CreateInfo.MutableUsage ) ); // 4.5

    return true;
}

bool Buffer::Orphan() {
    if ( CreateInfo.bImmutableStorage ) {
        LogPrintf( "Buffer::Orphan: expected mutable buffer\n" );
        return false;
    }

    glNamedBufferData( GL_HANDLE( Handle ), CreateInfo.SizeInBytes, nullptr, ChooseBufferUsageHint( CreateInfo.MutableClientAccess, CreateInfo.MutableUsage ) ); // 4.5

    return true;
}

void Buffer::Read( void * _SysMem ) {
    ReadRange( 0, CreateInfo.SizeInBytes, _SysMem );
}

void Buffer::ReadRange( size_t _ByteOffset, size_t _SizeInBytes, void * _SysMem ) {
    glGetNamedBufferSubData( GL_HANDLE( Handle ), _ByteOffset, _SizeInBytes, _SysMem ); // 4.5 or GL_ARB_direct_state_access

    /*
    // Other path:
    GLint id = GL_HANDLE( Handle );
    GLenum target = __BufferEnum[ CreateInfo.Type ].Target;
    GLint currentBinding;
    glGetIntegerv( __BufferEnum[ CreateInfo.Type ].Binding, &currentBinding );
    if ( currentBinding == id ) {
        glBufferSubData( target, _ByteOffset, _SizeInBytes, _SysMem );
    } else {
        glBindBuffer( GL_COPY_READ_BUFFER, id );
        glGetBufferSubData( GL_COPY_READ_BUFFER, _ByteOffset, _SizeInBytes, _SysMem );
    }
    */
}

void Buffer::Write( const void * _SysMem ) {
    WriteRange( 0, CreateInfo.SizeInBytes, _SysMem );
}

void Buffer::WriteRange( size_t _ByteOffset, size_t _SizeInBytes, const void * _SysMem ) {
    glNamedBufferSubData( GL_HANDLE( Handle ), _ByteOffset, _SizeInBytes, _SysMem ); // 4.5 or GL_ARB_direct_state_access

    /*
    // Other path:
    GLint id = GL_HANDLE( Handle );
    GLint currentBinding;
    GLenum target = __BufferEnum[ CreateInfo.Type ].Target;
    glGetIntegerv( __BufferEnum[ CreateInfo.Type ].Binding, &currentBinding );
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
    GLint id = GL_HANDLE( Handle );
    GLenum target = __BufferEnum[ CreateInfo.Type ].Target;
    GLint currentBinding;
    glGetIntegerv( __BufferEnum[ CreateInfo.Type ].Binding, &currentBinding );
    if ( currentBinding == id ) {
        glBufferSubData( target, _ByteOffset, _SizeInBytes, _SysMem );
    } else {
        glBindBuffer( GL_COPY_WRITE_BUFFER, id );
        glBufferSubData( GL_COPY_WRITE_BUFFER, _ByteOffset, _SizeInBytes, _SysMem );
    }
    */
}

void * Buffer::Map( MAP_TRANSFER _ClientServerTransfer,
                    MAP_INVALIDATE _Invalidate,
                    MAP_PERSISTENCE _Persistence,
                    bool _FlushExplicit,
                    bool _Unsynchronized ) {
    return MapRange( 0, CreateInfo.SizeInBytes,
                     _ClientServerTransfer,
                     _Invalidate,
                     _Persistence,
                     _FlushExplicit,
                     _Unsynchronized );
}

void * Buffer::MapRange( size_t _RangeOffset,
                         size_t _RangeLength,
                         MAP_TRANSFER _ClientServerTransfer,
                         MAP_INVALIDATE _Invalidate,
                         MAP_PERSISTENCE _Persistence,
                         bool _FlushExplicit,
                         bool _Unsynchronized ) {

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
        LogPrintf( "Buffer::MapRange: invalid map transfer function\n" );
        return nullptr;
    }

    if ( _Invalidate != MAP_NO_INVALIDATE ) {
        if ( flags & GL_MAP_READ_BIT ) {
            // This flag may not be used in combination with GL_MAP_READ_BIT.
            LogPrintf( "Buffer::MapRange: MAP_NO_INVALIDATE may not be used in combination with MAP_TRANSFER_READ/MAP_TRANSFER_RW\n" );
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

    return glMapNamedBufferRange( GL_HANDLE( Handle ), _RangeOffset, _RangeLength, flags );
}

void Buffer::Unmap() {
    glUnmapNamedBuffer( GL_HANDLE( Handle ) );
}

void * Buffer::GetMapPointer() {
    void * pointer = nullptr;
    glGetNamedBufferPointerv( GL_HANDLE( Handle ), GL_BUFFER_MAP_POINTER, &pointer );
    return pointer;
}

void Buffer::Invalidate() {
    glInvalidateBufferData( GL_HANDLE( Handle ) );
}

void Buffer::InvalidateRange( size_t _RangeOffset, size_t _RangeLength ) {
    glInvalidateBufferSubData( GL_HANDLE( Handle ), _RangeOffset, _RangeLength );
}

void Buffer::FlushMappedRange( size_t _RangeOffset, size_t _RangeLength ) {
    glFlushMappedNamedBufferRange( GL_HANDLE( Handle ), _RangeOffset, _RangeLength );
}

size_t Buffer::GetSizeOf( BUFFER_DATA_TYPE _DataType ) {
    TableBufferDataType const * type = &BufferDataTypeLUT[ _DataType ];

    return type->NumComponents * type->SizeOfComponent;
}




void ghi_create_buffer( ghi_buffer_t * buffer, ghi_buffer_create_info_t const * create_info, const void * sys_mem ) {
    GLuint id;
    GLint size;

    glCreateBuffers( 1, &id );

    if ( create_info->bImmutableStorage ) {
        glNamedBufferStorage( id, create_info->SizeInBytes, sys_mem, create_info->ImmutableStorageFlags ); // 4.5 or GL_ARB_direct_state_access
        //glBufferStorage // 4.4 or GL_ARB_buffer_storage
    } else {
        glNamedBufferData( id, create_info->SizeInBytes, sys_mem, ChooseBufferUsageHint( create_info->MutableClientAccess, create_info->MutableUsage ) ); // 4.5 or GL_ARB_direct_state_access
    }

    glGetNamedBufferParameteriv( id, GL_BUFFER_SIZE, &size );

    if ( size != (GLint)create_info->SizeInBytes ) {
        glDeleteBuffers( 1, &id );

        LogPrintf( "Buffer::Initialize: couldn't allocate buffer size %u bytes\n", create_info->SizeInBytes );
        return;
    }

    buffer->device = ghi_get_current_state()->device;
    buffer->handle = ( void * )( size_t )id;
    buffer->uid = ghi_internal_GenerateUID( buffer->device );
    buffer->immutable = create_info->bImmutableStorage;
    buffer->immutable_flags = create_info->ImmutableStorageFlags;
    buffer->mutable_client_access = create_info->MutableClientAccess;
    buffer->mutable_usage = create_info->MutableUsage;
    buffer->size = create_info->SizeInBytes;

    buffer->device->TotalBuffers++;
    buffer->device->BufferMemoryAllocated += create_info->SizeInBytes;
}

void ghi_destroy_buffer( ghi_buffer_t * buffer ) {
    GLuint id = GL_HANDLE( buffer->handle );
    glDeleteBuffers( 1, &id );
    buffer->device->TotalBuffers--;
    buffer->device->BufferMemoryAllocated -= buffer->size;
}

bool ghi_buffer_realloc( ghi_buffer_t * buffer, size_t size, const void * sys_mem ) {
    if ( buffer->immutable ) {
        LogPrintf( "Buffer::Realloc: immutable buffer cannot be reallocated\n" );
        return false;
    }

    buffer->size = size;

    glNamedBufferData( GL_HANDLE( buffer->handle ), size, sys_mem, ChooseBufferUsageHint( buffer->mutable_client_access, buffer->mutable_usage ) ); // 4.5

    return true;
}

bool ghi_buffer_orphan( ghi_buffer_t * buffer ) {
    if ( buffer->immutable ) {
        LogPrintf( "Buffer::Orphan: expected mutable buffer\n" );
        return false;
    }

    glNamedBufferData( GL_HANDLE( buffer->handle ), buffer->size, nullptr, ChooseBufferUsageHint( buffer->mutable_client_access, buffer->mutable_usage ) ); // 4.5

    return true;
}

void ghi_buffer_read( ghi_buffer_t * buffer, void * sys_mem ) {
    ghi_buffer_read_range( buffer, 0, buffer->size, sys_mem );
}

void ghi_buffer_read_range( ghi_buffer_t * buffer, size_t offset, size_t size, void * sys_mem ) {
    glGetNamedBufferSubData( GL_HANDLE( buffer->handle ), offset, size, sys_mem ); // 4.5 or GL_ARB_direct_state_access
}

void ghi_buffer_write( ghi_buffer_t * buffer, const void * sys_mem ) {
    ghi_buffer_write_range( buffer, 0, buffer->size, sys_mem );
}

void ghi_buffer_write_range( ghi_buffer_t * buffer, size_t offset, size_t size, const void * sys_mem ) {
    glNamedBufferSubData( GL_HANDLE( buffer->handle ), offset, size, sys_mem ); // 4.5 or GL_ARB_direct_state_access
}

void * ghi_buffer_map_range( ghi_buffer_t * buffer, size_t offset, size_t size, MAP_TRANSFER transfer, MAP_INVALIDATE invalidate, MAP_PERSISTENCE persistence, bool flush_explicit, bool unsynchronized ) {
    int flags = 0;

    switch ( transfer ) {
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
        LogPrintf( "Buffer::MapRange: invalid map transfer function\n" );
        return nullptr;
    }

    if ( invalidate != MAP_NO_INVALIDATE ) {
        if ( flags & GL_MAP_READ_BIT ) {
            // This flag may not be used in combination with GL_MAP_READ_BIT.
            LogPrintf( "Buffer::MapRange: MAP_NO_INVALIDATE may not be used in combination with MAP_TRANSFER_READ/MAP_TRANSFER_RW\n" );
            return nullptr;
        }

        if ( invalidate == MAP_INVALIDATE_ENTIRE_BUFFER ) {
            flags |= GL_MAP_INVALIDATE_BUFFER_BIT;
        } else if ( invalidate == MAP_INVALIDATE_RANGE ) {
            flags |= GL_MAP_INVALIDATE_RANGE_BIT;
        }
    }

    switch ( persistence ) {
    case MAP_NON_PERSISTENT:
        break;
    case MAP_PERSISTENT_COHERENT:
        flags |= GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        break;
    case MAP_PERSISTENT_NO_COHERENT:
        flags |= GL_MAP_PERSISTENT_BIT;
        break;
    }

    if ( flush_explicit ) {
        flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
    }

    if ( unsynchronized ) {
        flags |= GL_MAP_UNSYNCHRONIZED_BIT;
    }

    return glMapNamedBufferRange( GL_HANDLE( buffer->handle ), offset, size, flags );
}

void * ghi_buffer_map( ghi_buffer_t * buffer, MAP_TRANSFER transfer, MAP_INVALIDATE invalidate, MAP_PERSISTENCE persistence, bool flush_explicit, bool unsynchronized ) {
    return ghi_buffer_map_range( buffer, 0, buffer->size, transfer, invalidate, persistence, flush_explicit, unsynchronized );
}

void ghi_buffer_unmap( ghi_buffer_t * buffer ) {
    glUnmapNamedBuffer( GL_HANDLE( buffer->handle ) );
}

void * ghi_buffer_get_map_pointer( ghi_buffer_t * buffer ) {
    void * pointer = NULL;
    glGetNamedBufferPointerv( GL_HANDLE( buffer->handle ), GL_BUFFER_MAP_POINTER, &pointer );
    return pointer;
}

void ghi_buffer_invalidate( ghi_buffer_t * buffer ) {
    glInvalidateBufferData( GL_HANDLE( buffer->handle ) );
}

void ghi_buffer_invalidate_range( ghi_buffer_t * buffer, size_t offset, size_t size ) {
    glInvalidateBufferSubData( GL_HANDLE( buffer->handle ), offset, size );
}

void ghi_buffer_flush_mapped_range( ghi_buffer_t * buffer, size_t offset, size_t size ) {
    glFlushMappedNamedBufferRange( GL_HANDLE( buffer->handle ), offset, size );
}


}
