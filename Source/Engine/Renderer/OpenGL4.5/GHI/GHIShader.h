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

#pragma once

#include "GHIBasic.h"

namespace GHI {

class Device;

enum SHADER_TYPE : uint8_t
{
    VERTEX_SHADER,
    FRAGMENT_SHADER,
    TESS_CONTROL_SHADER,
    TESS_EVALUATION_SHADER,
    GEOMETRY_SHADER,
    COMPUTE_SHADER
};

struct ShaderBinaryData {
    void *       BinaryCode;
    size_t       BinaryLength;
    unsigned int BinaryFormat;
    SHADER_TYPE  ShaderType;
};

class ShaderModule final : public NonCopyable, IObjectInterface {
public:
    ShaderModule();
    ~ShaderModule();

    bool InitializeFromBinary( ShaderBinaryData const * _BinaryData, char ** _InfoLog = nullptr );
    bool InitializeFromCode( SHADER_TYPE _ShaderType, unsigned int _NumSources, const char * const * _Sources, char ** _InfoLog = nullptr );
    bool InitializeFromCode( SHADER_TYPE _ShaderType, const char * _Source, char ** _InfoLog = nullptr );

    void Deinitialize();

    SHADER_TYPE GetType() const { return Type; }

    void * GetHandle() const { return Handle; }

    // Experemental
    void SetUniform2f( int location, float v0, float v1 );
    void SetUniform3f( int location, float v0, float v1, float v2 );

    //
    // Utilites
    //

    static bool CreateBinaryData( SHADER_TYPE _ShaderType,
                                  unsigned int _NumSources,
                                  const char * const * _Sources,
                                  /* optional */ char ** _InfoLog,
                                  AllocatorCallback const & _Allocator,
                                  ShaderBinaryData * _BinaryData );

    static void DestroyBinaryData( AllocatorCallback const & _Allocator, ShaderBinaryData * _BinaryData );

private:
    Device * pDevice;
    void * Handle;
    SHADER_TYPE Type;
};

}
