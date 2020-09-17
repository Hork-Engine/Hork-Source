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

#include "ShaderModuleGLImpl.h"
#include "DeviceGLImpl.h"
#include "ImmediateContextGLImpl.h"
#include "LUT.h"
#include "GL/glew.h"

#include <Core/Public/CString.h>
#include <Core/Public/Logger.h>

namespace RenderCore {

static GLuint CreateShaderProgramBin( SShaderBinaryData const * _BinaryData, char ** _InfoLog = nullptr ) {
    static char ErrorLog[ MAX_ERROR_LOG_LENGTH ];

    ErrorLog[0] = 0;

    if ( _InfoLog ) {
        *_InfoLog = ErrorLog;
    }

    GLuint program = glCreateProgram();
    if ( !program ) {

        if ( _InfoLog ) {
            Core::Strcpy( ErrorLog, sizeof( ErrorLog ), "Failed to create shader program" );
        }

        return 0;
    }

    glProgramParameteri( program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_FALSE );
    //glProgramParameteri( program, GL_PROGRAM_SEPARABLE, GL_TRUE );

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

AShaderModuleGLImpl::AShaderModuleGLImpl( ADeviceGLImpl * _Device, SShaderBinaryData const * _BinaryData, char ** _InfoLog )
    : pDevice( _Device )
{
    GLuint id = CreateShaderProgramBin( _BinaryData, _InfoLog );
    if ( !id ) {
        GLogger.Printf( "AShaderModuleGLImpl::ctor: invalid binary data\n" );
        return;
    }

    SetHandleNativeGL( id );
    Type = _BinaryData->ShaderType;

    pDevice->TotalShaderModules++;
}

AShaderModuleGLImpl::AShaderModuleGLImpl( ADeviceGLImpl * _Device, SHADER_TYPE _ShaderType, unsigned int _NumSources, const char * const * _Sources, char ** _InfoLog )
    : pDevice( _Device )
{
#if 0
    AllocatorCallback const & allocator = pDevice->GetAllocator();
    ShaderBinaryData binaryData;

    if ( !CreateBinaryData( _ShaderType, _NumSources, _Sources, _InfoLog, allocator, &binaryData ) ) {
        GLogger.Printf( "AShaderModuleGLImpl::ctor: couldn't create shader binary data\n" );
        return false;
    }

    bool result = InitializeFromBinary( &binaryData, _InfoLog );
    DestroyBinaryData( allocator, &binaryData );
    return result;
#else

    GLuint id = pDevice->CreateShaderProgram( ShaderTypeLUT[_ShaderType], _NumSources, _Sources, _InfoLog );
    if ( !id ) {
        GLogger.Printf( "AShaderModuleGLImpl::ctor: couldn't create shader program\n" );
        return;
    }

    SetHandleNativeGL( id );
    Type = _ShaderType;

    pDevice->TotalShaderModules++;
#endif
}

AShaderModuleGLImpl::AShaderModuleGLImpl( ADeviceGLImpl * _Device, SHADER_TYPE _ShaderType, const char * _Source, char ** _InfoLog )
    : AShaderModuleGLImpl( _Device, _ShaderType, 1, &_Source, _InfoLog )
{
}

AShaderModuleGLImpl::~AShaderModuleGLImpl() {
    GLuint id = GetHandleNativeGL();

    if ( !id ) {
        return;
    }

    pDevice->DeleteShaderProgram( id );
    pDevice->TotalShaderModules--;
}

}
