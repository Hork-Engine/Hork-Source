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

#pragma once

#include "OpenGL45Common.h"
#include <Core/Public/Logger.h>

namespace OpenGL45 {

struct AShaderSources {
    enum { MAX_SOURCES = 10 };
    const char * Sources[MAX_SOURCES];
    int NumSources;

    void Clear() {
        NumSources = 2;
    }

    void Add( const char * _Source ) {
        AN_ASSERT( NumSources < MAX_SOURCES );

        if ( NumSources < MAX_SOURCES ) {
            Sources[NumSources++] = _Source;
        }
    }

    void Build( GHI::SHADER_TYPE _ShaderType, GHI::ShaderModule * _Module ) {
        using namespace GHI;

        char * Log = nullptr;

        const char * predefine[] = {
            "#define VERTEX_SHADER\n",
            "#define FRAGMENT_SHADER\n",
            "#define TESS_CONTROL_SHADER\n",
            "#define TESS_EVALUATION_SHADER\n",
            "#define GEOMETRY_SHADER\n",
            "#define COMPUTE_SHADER\n"
        };

        AString predefines = predefine[_ShaderType];
        predefines += "#define MAX_DIRECTIONAL_LIGHTS " + Math::ToString( MAX_DIRECTIONAL_LIGHTS ) + "\n";
        predefines += "#define MAX_SHADOW_CASCADES " + Math::ToString( MAX_SHADOW_CASCADES ) + "\n";

        #ifdef SHADOWMAP_PCF
        predefines += "#define SHADOWMAP_PCF\n";
        #endif
        #ifdef SHADOWMAP_PCSS
        predefines += "#define SHADOWMAP_PCSS\n";
        #endif
        #ifdef SHADOWMAP_VSM
        predefines += "#define SHADOWMAP_VSM\n";
        #endif
        #ifdef SHADOWMAP_EVSM
        predefines += "#define SHADOWMAP_EVSM\n";
        #endif
        #ifdef AN_DEBUG
        predefines += "#define DEBUG_RENDER_MODE\n";
        #endif

        Sources[0] = "#version 450\n"
                     "#extension GL_ARB_bindless_texture : enable\n";
        Sources[1] = predefines.CStr();

        _Module->InitializeFromCode( _ShaderType, NumSources, Sources, &Log );

        if ( Log && *Log ) {
            switch ( _ShaderType ) {
            case VERTEX_SHADER:
                GLogger.Printf( "VS: %s\n", Log );
                break;
            case FRAGMENT_SHADER:
                GLogger.Printf( "FS: %s\n", Log );
                break;
            case TESS_CONTROL_SHADER:
                GLogger.Printf( "TCS: %s\n", Log );
                break;
            case TESS_EVALUATION_SHADER:
                GLogger.Printf( "TES: %s\n", Log );
                break;
            case GEOMETRY_SHADER:
                GLogger.Printf( "GS: %s\n", Log );
                break;
            case COMPUTE_SHADER:
                GLogger.Printf( "CS: %s\n", Log );
                break;
            }
        }
    }

    void PrintSources() {
        for ( int i = 0 ; i < NumSources ; i++ ) {
            GLogger.Printf( "%s\n", i, Sources[i] );
        }
    }
};

extern AShaderSources GShaderSources;

}
