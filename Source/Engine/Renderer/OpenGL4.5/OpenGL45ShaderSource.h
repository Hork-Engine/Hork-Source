/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

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

constexpr const char * UniformStr = AN_STRINGIFY(

layout( binding = 0, std140 ) uniform UniformBuffer0
{
    mat4 OrthoProjection;
    mat4 ModelviewProjection;
    mat4 InverseProjectionMatrix;
    vec4 WorldNormalToViewSpace0;
    vec4 WorldNormalToViewSpace1;
    vec4 WorldNormalToViewSpace2;
    vec4 ViewportParams;  // 1/width, 1/height, znear, zfar
    vec4 Timers;
    vec4 ViewPostion;
    uvec2 EnvProbeSampler;
    int  NumDirectionalLights;
    vec4 LightDirs[MAX_DIRECTIONAL_LIGHTS];            // Direction, W-channel is not used
    vec4 LightColors[MAX_DIRECTIONAL_LIGHTS];          // RGB, alpha - ambient intensity
    uvec4 LightParameters[MAX_DIRECTIONAL_LIGHTS];    // RenderMask, FirstCascade, NumCascades, W-channel is not used
};

);

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

        char * Log;

        const char * predefine[] = {
            "#define VERTEX_SHADER\n",
            "#define FRAGMENT_SHADER\n",
            "#define TESS_CONTROL_SHADER\n",
            "#define TESS_EVALUATION_SHADER\n",
            "#define GEOMETRY_SHADER\n",
            "#define COMPUTE_SHADER\n"
        };

        AString predefines = predefine[_ShaderType];
        predefines += "#define MAX_DIRECTIONAL_LIGHTS " + Int( MAX_DIRECTIONAL_LIGHTS ).ToString() + "\n";
        predefines += "#define MAX_SHADOW_CASCADES " + Int( MAX_SHADOW_CASCADES ).ToString() + "\n";

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

        Sources[0] = "#version 450\n"
                     "#extension GL_ARB_bindless_texture : enable\n";
        Sources[1] = predefines.CStr();

        _Module->InitializeFromCode( _ShaderType, NumSources, Sources, &Log );

        if ( *Log ) {
            //for ( int i = 0 ; i < NumSources ; i++ ) {
            //    GLogger.Print( Sources[i] );
            //}
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
};

extern AShaderSources GShaderSources;

}
