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

#include "GHIShader.h"
#include "GHIDevice.h"
#include "GHIState.h"
#include "LUT.h"

#include <string>
#include <memory.h>
#include "GL/glew.h"

namespace GHI {

#define MAX_ERROR_LOG_LENGTH  2048

static void Strcpy( char * _Dest, size_t _DestSz, const char * _Src ) {
#ifdef _MSC_VER
    strcpy_s( _Dest, _DestSz, _Src );
#else
    if ( _DestSz > 0 ) {
        while ( *_Src && --_DestSz != 0 ) {
            *_Dest++ = *_Src++;
        }
        *_Dest = 0;
    }
#endif
}

ShaderModule::ShaderModule() {
    pDevice = nullptr;
    Handle = nullptr;
}

ShaderModule::~ShaderModule() {
    Deinitialize();
}

static GLuint CreateShaderProgram( GLenum _Type,
                                   GLsizei _NumStrings,
                                   const GLchar * const * _Strings,
                                   char ** _InfoLog = nullptr ) {
    static char ErrorLog[ MAX_ERROR_LOG_LENGTH ];
    GLuint program;

    ErrorLog[0] = 0;

    if ( _InfoLog ) {
        *_InfoLog = ErrorLog;
    }

#if 1
    program = glCreateShaderProgramv( _Type, _NumStrings, _Strings );  // v 4.1
    if ( program ) {
        glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE );
    }

#else
    // Other path
    GLuint shader = glCreateShader( _Type );
    if ( shader ) {
        glShaderSource( shader, _NumStrings, _Strings, NULL );
        glCompileShader( shader );
        program = glCreateProgram();
        if ( program ) {
            GLint compiled = GL_FALSE;
            glGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
            glProgramParameteri( program, GL_PROGRAM_SEPARABLE, GL_TRUE );
            glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE );
            if ( compiled ) {
                glAttachShader( program, shader );
                glLinkProgram( program );
                glDetachShader( program, shader );
            }
            /* append-shader-info-log-to-program-info-log */
        }
        glDeleteShader( shader );
    } else {
        program = 0;
    }
#endif

    if ( !program ) {

        if ( _InfoLog ) {
            Strcpy( ErrorLog, sizeof( ErrorLog ), "Failed to create shader program" );
        }

        return 0;
    }

    GLint linkStatus = 0;
    glGetProgramiv( program, GL_LINK_STATUS, &linkStatus );
    if ( !linkStatus ) {

        if ( _InfoLog ) {
            GLint infoLogLength = 0;
            glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infoLogLength );
            glGetProgramInfoLog( program, sizeof( ErrorLog ), NULL, ErrorLog );

            if ( (GLint)sizeof( ErrorLog ) < infoLogLength ) {
                ErrorLog[ sizeof( ErrorLog ) - 4 ] = '.';
                ErrorLog[ sizeof( ErrorLog ) - 3 ] = '.';
                ErrorLog[ sizeof( ErrorLog ) - 2 ] = '.';
            }
        }

        glDeleteProgram( program );

        return 0;
    }

    return program;
}

static GLuint CreateShaderProgramBin( ShaderBinaryData const * _BinaryData, char ** _InfoLog = nullptr ) {
    static char ErrorLog[ MAX_ERROR_LOG_LENGTH ];

    ErrorLog[0] = 0;

    if ( _InfoLog ) {
        *_InfoLog = ErrorLog;
    }

    GLuint program = glCreateProgram();
    if ( !program ) {

        if ( _InfoLog ) {
            Strcpy( ErrorLog, sizeof( ErrorLog ), "Failed to create shader program" );
        }

        return 0;
    }

    glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_FALSE );

    glProgramBinary( program, _BinaryData->BinaryFormat, _BinaryData->BinaryCode, (GLsizei)_BinaryData->BinaryLength );

    GLint linkStatus = 0;
    glGetProgramiv( program, GL_LINK_STATUS, &linkStatus );
    if ( !linkStatus ) {

        if ( _InfoLog ) {
            GLint infoLogLength = 0;
            glGetProgramiv( program, GL_INFO_LOG_LENGTH, &infoLogLength );
            glGetProgramInfoLog( program, sizeof( ErrorLog ), NULL, ErrorLog );

            if ( (GLint)sizeof( ErrorLog ) < infoLogLength ) {
                ErrorLog[ sizeof( ErrorLog ) - 4 ] = '.';
                ErrorLog[ sizeof( ErrorLog ) - 3 ] = '.';
                ErrorLog[ sizeof( ErrorLog ) - 2 ] = '.';
            }
        }

        glDeleteProgram( program );

        return 0;
    }

    return program;
}

static void DeleteShaderProgram( GLuint _Program ) {
    glDeleteProgram( _Program );
}


bool ShaderModule::InitializeFromBinary( ShaderBinaryData const * _BinaryData, char ** _InfoLog ) {
    State * state = GetCurrentState();

    Deinitialize();

    GLuint id = CreateShaderProgramBin( _BinaryData, _InfoLog );
    if ( !id ) {
        LogPrintf( "ShaderModule::InitializeFromBinary: invalid binary data\n" );
        return false;
    }

    Handle = ( void * )( size_t )id;
    Type = _BinaryData->ShaderType;

    pDevice = state->GetDevice();
    pDevice->TotalShaderModules++;

    return true;
}

bool ShaderModule::InitializeFromCode( SHADER_TYPE _ShaderType, unsigned int _NumSources, const char * const * _Sources, char ** _InfoLog ) {
    State * state = GetCurrentState();
    AllocatorCallback const & allocator = state->GetDevice()->GetAllocator();

    ShaderBinaryData binaryData;

    if ( !CreateBinaryData( _ShaderType, _NumSources, _Sources, _InfoLog, allocator, &binaryData ) ) {
        LogPrintf( "ShaderModule::InitializeFromBinary: couldn't create shader binary data\n" );
        return false;
    }

    bool result = InitializeFromBinary( &binaryData, _InfoLog );

    DestroyBinaryData( allocator, &binaryData );
    return result;
}

bool ShaderModule::InitializeFromCode( SHADER_TYPE _ShaderType, const char * _Source, char ** _InfoLog ) {
    return InitializeFromCode( _ShaderType, 1, &_Source, _InfoLog );
}

void ShaderModule::Deinitialize() {
    if ( !Handle ) {
        return;
    }

    DeleteShaderProgram( GL_HANDLE( Handle ) );
    pDevice->TotalShaderModules--;

    Handle = nullptr;
    pDevice = nullptr;
}

void ShaderModule::SetUniform2f( int location, float v0, float v1 ) {
    glProgramUniform2f( GL_HANDLE( Handle ), location, v0, v1 );
}

bool ShaderModule::CreateBinaryData( SHADER_TYPE _ShaderType,
                                     unsigned int _NumSources,
                                     const char * const * _Sources,
                                     /* optional */ char ** _InfoLog,
                                     AllocatorCallback const & _Allocator,
                                     ShaderBinaryData * _BinaryData ) {
    GLuint id;
    GLsizei binaryLength;
    GLsizei length;
    GLenum format;
    uint8_t * binary;

    memset( _BinaryData, 0, sizeof( *_BinaryData ) );

    id = CreateShaderProgram( ShaderTypeLUT[ _ShaderType ], _NumSources, _Sources, _InfoLog );
    if ( !id ) {
        LogPrintf( "ShaderModule::CreateBinaryData: couldn't create shader program\n" );
        return false;
    }

    glGetProgramiv( id, GL_PROGRAM_BINARY_LENGTH, &binaryLength );
    if ( binaryLength ) {
        binary = ( uint8_t * )_Allocator.Allocate( binaryLength );

        glGetProgramBinary( id, binaryLength, &length, &format, binary );

        _BinaryData->BinaryCode = binary;
        _BinaryData->BinaryLength = length;
        _BinaryData->BinaryFormat = format;
        _BinaryData->ShaderType = _ShaderType;
    } else {
        if ( _InfoLog ) {
            Strcpy( *_InfoLog, MAX_ERROR_LOG_LENGTH, "Failed to retrieve shader program binary length" );
        }

        DeleteShaderProgram( id );
        return false;
    }

    DeleteShaderProgram( id );
    return true;
}

void ShaderModule::DestroyBinaryData( AllocatorCallback const & _Allocator, ShaderBinaryData * _BinaryData ) {
    if ( !_BinaryData->BinaryCode ) {
        return;
    }

    uint8_t * binary = ( uint8_t * )_BinaryData->BinaryCode;
    _Allocator.Deallocate( binary );

    memset( _BinaryData, 0, sizeof( *_BinaryData ) );
}

}
