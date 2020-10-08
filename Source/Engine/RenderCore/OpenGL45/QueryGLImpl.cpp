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

#include "QueryGLImpl.h"
#include "DeviceGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

#include <Core/Public/Logger.h>

namespace RenderCore {

AQueryPoolGLImpl::AQueryPoolGLImpl( ADeviceGLImpl * _Device, SQueryPoolCreateInfo const & _CreateInfo )
    : IQueryPool( _Device ), pDevice( _Device )
{
    AN_ASSERT( _CreateInfo.PoolSize > 0 );

    IdPool = nullptr;
    QueryType = _CreateInfo.QueryType;
    PoolSize = _CreateInfo.PoolSize;

    SAllocatorCallback const & allocator = pDevice->GetAllocator();

    IdPool = ( unsigned int * )allocator.Allocate( sizeof( *IdPool ) * PoolSize );

    if ( !IdPool ) {
        GLogger.Printf( "AQueryPoolGLImpl::ctor: out of memory\n" );
        return;
    }

    // TODO: create queries for each context
    glCreateQueries( TableQueryTarget[QueryType], PoolSize, IdPool ); // 4.5

    pDevice->TotalQueryPools++;

    SetHandleNativeGL( IdPool[0] );
}

AQueryPoolGLImpl::~AQueryPoolGLImpl()
{
    if ( !IdPool ) {
        return;
    }

    glDeleteQueries( PoolSize, IdPool ); // 4.5
    pDevice->TotalQueryPools--;
    pDevice->GetAllocator().Deallocate( IdPool );
}

void AQueryPoolGLImpl::GetResults( uint32_t _FirstQuery,
                                   uint32_t _QueryCount,
                                   size_t _DataSize,
                                   void * _SysMem,
                                   size_t _DstStride,
                                   QUERY_RESULT_FLAGS _Flags )
{
    AN_ASSERT( _FirstQuery + _QueryCount <= PoolSize );

    glBindBuffer( GL_QUERY_BUFFER, 0 );

    uint8_t * ptr = ( uint8_t * )_SysMem;
    uint8_t * end = ptr + _DataSize;

    if ( _Flags & QUERY_RESULT_64_BIT ) {

        AN_ASSERT( ( _DstStride & ~(size_t)7 ) == _DstStride ); // check stride must be multiples of 8

        for ( uint32_t index = 0 ; index < _QueryCount ; index++ ) {

            if ( ptr + sizeof(uint64_t) > end ) {
                GLogger.Printf( "QueryPool::GetResults: out of data size\n" );
                break;
            }

            if ( _Flags & QUERY_RESULT_WAIT_BIT ) {
                glGetQueryObjectui64v( IdPool[ _FirstQuery + index ], GL_QUERY_RESULT, (GLuint64 *)ptr ); // 3.2

                if ( _Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT ) {
                    *(uint64_t *)ptr |= 0x8000000000000000;
                }
            } else {
                if ( _Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT ) {
                    uint64_t available;
                    glGetQueryObjectui64v( IdPool[ _FirstQuery + index ], GL_QUERY_RESULT_AVAILABLE, &available ); // 3.2

                    if ( available ) {
                        glGetQueryObjectui64v( IdPool[ _FirstQuery + index ], GL_QUERY_RESULT, (GLuint64 *)ptr ); // 3.2
                        *(uint64_t *)ptr |= 0x8000000000000000;
                    } else {
                        *(uint64_t *)ptr = 0;
                    }
                } else {
                    *(uint64_t *)ptr = 0;
                    glGetQueryObjectui64v( IdPool[ _FirstQuery + index ], GL_QUERY_RESULT_NO_WAIT, (GLuint64 *)ptr ); // 3.2
                }
            }

            ptr += _DstStride;
        }
    } else {

        AN_ASSERT( ( _DstStride & ~(size_t)3 ) == _DstStride ); // check stride must be multiples of 4

        for ( uint32_t index = 0 ; index < _QueryCount ; index++ ) {

            if ( ptr + sizeof(uint32_t) > end ) {
                GLogger.Printf( "QueryPool::GetResults: out of data size\n" );
                break;
            }

            if ( _Flags & QUERY_RESULT_WAIT_BIT ) {
                glGetQueryObjectuiv( IdPool[ _FirstQuery + index ], GL_QUERY_RESULT, (GLuint *)ptr ); // 2.0

                if ( _Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT ) {
                    *(uint32_t *)ptr |= 0x80000000;
                }
            } else {
                if ( _Flags & QUERY_RESULT_WITH_AVAILABILITY_BIT ) {
                    uint32_t available;
                    glGetQueryObjectuiv( IdPool[ _FirstQuery + index ], GL_QUERY_RESULT_AVAILABLE, &available ); // 2.0

                    if ( available ) {
                        glGetQueryObjectuiv( IdPool[ _FirstQuery + index ], GL_QUERY_RESULT, (GLuint *)ptr ); // 2.0
                        *(uint32_t *)ptr |= 0x80000000;
                    } else {
                        *(uint32_t *)ptr = 0;
                    }
                } else {
                    *(uint32_t *)ptr = 0;
                    glGetQueryObjectuiv( IdPool[ _FirstQuery + index ], GL_QUERY_RESULT_NO_WAIT, (GLuint *)ptr ); // 2.0
                }
            }

            ptr += _DstStride;
        }
    }
}

}
