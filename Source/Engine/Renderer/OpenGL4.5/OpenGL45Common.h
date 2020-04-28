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

#include "GHI/GHIPipeline.h"
#include "GHI/GHIShader.h"
#include "GHI/GHISampler.h"
#include "GHI/GHIBuffer.h"
#include "GHI/GHITexture.h"
#include "GHI/GHICommandBuffer.h"
#include "GHI/GHIDevice.h"
#include "GHI/GHIState.h"
#include "GHI/GHIFramebuffer.h"
#include "GHI/GHIPipeline.h"
#include "GHI/GHIRenderPass.h"

#include <Runtime/Public/RenderCore.h>
#include <Runtime/Public/RuntimeVariable.h>

#include <Core/Public/Logger.h>

#define SCISSOR_TEST false
#define DEPTH_PREPASS

#define SHADOWMAP_PCF
//#define SHADOWMAP_PCSS
//#define SHADOWMAP_VSM
//#define SHADOWMAP_EVSM

extern ARuntimeVariable RVRenderSnapshot;

namespace OpenGL45 {

extern GHI::Device          GDevice;
extern GHI::State           GState;
extern GHI::CommandBuffer   Cmd;
extern SRenderFrame *       GFrameData;
extern SRenderView *        GRenderView;

AN_FORCEINLINE GHI::Buffer * GPUBufferHandle( ABufferGPU * _Buffer ) {
 
    return static_cast< GHI::Buffer * >( _Buffer->pHandleGPU );
}

AN_FORCEINLINE GHI::Texture * GPUTextureHandle( ATextureGPU * _Texture ) {
    return static_cast< GHI::Texture * >( _Texture->pHandleGPU );
}

void SaveSnapshot( GHI::Texture & _Texture );

AString LoadShader( const char * FileName, SMaterialShader const * Predefined = nullptr );
AString LoadShaderFromString( const char * FileName, const char * Source, SMaterialShader const * Predefined = nullptr );

void CreateFullscreenQuadPipeline( GHI::Pipeline & Pipe, const char * VertexShader, const char * FragmentShader, GHI::RenderPass & Pass, GHI::BLENDING_PRESET BlendingPreset = GHI::BLENDING_NO_BLEND );

void CreateFullscreenQuadPipelineGS( GHI::Pipeline & Pipe, const char * VertexShader, const char * FragmentShader, const char * GeometryShader, GHI::RenderPass & Pass, GHI::BLENDING_PRESET BlendingPreset = GHI::BLENDING_NO_BLEND );

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

        predefines += "#define SRGB_GAMMA_APPROX\n";

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
