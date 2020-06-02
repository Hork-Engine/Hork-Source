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

#include "FrameGraph.h"

#define SCISSOR_TEST false
#define DEPTH_PREPASS

#define SHADOWMAP_PCF
//#define SHADOWMAP_PCSS
//#define SHADOWMAP_VSM
//#define SHADOWMAP_EVSM

extern ARuntimeVariable RVRenderSnapshot;

namespace OpenGL45 {

AN_FORCEINLINE GHI::Buffer * GPUBufferHandle( ABufferGPU * _Buffer )
{ 
    return static_cast< GHI::Buffer * >( _Buffer->pHandleGPU );
}

AN_FORCEINLINE GHI::Texture * GPUTextureHandle( ATextureGPU * _Texture )
{
    return static_cast< GHI::Texture * >( _Texture->pHandleGPU );
}

GHI::TextureResolution2D GetFrameResoultion();

void DrawSAQ( GHI::Pipeline * Pipeline );

void BindVertexAndIndexBuffers( SRenderInstance const * _Instance );

void BindVertexAndIndexBuffers( SShadowRenderInstance const * _Instance );

void BindSkeleton( size_t _Offset, size_t _Size );

void BindTextures( SMaterialFrameData * _MaterialInstance );

void SetInstanceUniforms( SRenderInstance const * Instance, int _Index );

void SetShadowInstanceUniforms( SShadowRenderInstance const * Instance, int _Index );

void SaveSnapshot( GHI::Texture & _Texture );

AString LoadShader( const char * FileName, SMaterialShader const * Predefined = nullptr );
AString LoadShaderFromString( const char * FileName, const char * Source, SMaterialShader const * Predefined = nullptr );

void CreateFullscreenQuadPipeline( GHI::Pipeline & Pipe, const char * VertexShader, const char * FragmentShader, GHI::BLENDING_PRESET BlendingPreset = GHI::BLENDING_NO_BLEND,
                                   GHI::ShaderModule * pVertexShaderModule = NULL, GHI::ShaderModule * pFragmentShaderModule = NULL );

void CreateFullscreenQuadPipelineGS( GHI::Pipeline & Pipe, const char * VertexShader, const char * FragmentShader, const char * GeometryShader, GHI::BLENDING_PRESET BlendingPreset = GHI::BLENDING_NO_BLEND,
                                     GHI::ShaderModule * pVertexShaderModule = NULL, GHI::ShaderModule * pFragmentShaderModule = NULL, GHI::ShaderModule * pGeometryShaderModule = NULL );

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

class ACircularBuffer {
public:
    ACircularBuffer( size_t InBufferSize );
    virtual ~ACircularBuffer();

    size_t Allocate( size_t InSize );

    byte * GetMappedMemory() { return (byte *)pMappedMemory; }

    GHI::Buffer * GetBuffer() { return &Buffer; }

private:
    struct SChainBuffer {
        size_t UsedMemory;
        GHI::SyncObject Sync;
    };

    SChainBuffer * Swap();

    void Wait( GHI::SyncObject Sync );

    GHI::Buffer Buffer;
    void * pMappedMemory;
    int BufferIndex;

    enum { SWAP_CHAIN_SIZE = 3 };

    SChainBuffer ChainBuffer[SWAP_CHAIN_SIZE];
    size_t BufferSize;
};

class AFrameConstantBuffer {
public:
    AFrameConstantBuffer( size_t InBufferSize );
    virtual ~AFrameConstantBuffer();

    size_t Allocate( size_t InSize );

    byte * GetMappedMemory() { return (byte *)pMappedMemory; }

    GHI::Buffer * GetBuffer() { return &Buffer; }

    void Begin();
    void End();

private:
    struct SChainBuffer {
        size_t UsedMemory;
        GHI::SyncObject Sync;
    };

    void Wait( GHI::SyncObject Sync );

    GHI::Buffer Buffer;
    void * pMappedMemory;
    int BufferIndex;

    enum { SWAP_CHAIN_SIZE = 3 };

    SChainBuffer ChainBuffer[SWAP_CHAIN_SIZE];
    size_t BufferSize;
};

struct SViewUniformBuffer {
    Float4x4 OrthoProjection;
    Float4x4 ViewProjection;
    Float4x4 InverseProjectionMatrix;
    Float3x4 WorldNormalToViewSpace;

    // ViewportParams
    Float2 InvViewportSize;
    float ZNear;
    float ZFar;

    // Timers
    float GameRunningTimeSeconds;
    float GameplayTimeSeconds;
    float DynamicResolutionRatioX;
    float DynamicResolutionRatioY;

    Float3 ViewPosition;
    float TimeDelta;

    Float4 PostprocessBloomMix;

    // PostprocessAttrib
    float BloomEnabled;
    float ToneMappingExposure;
    float ColorGrading;
    float FXAA;

    Float4 VignetteColorIntensity;        // rgb, intensity
    float VignetteOuterRadiusSqr;
    float VignetteInnerRadiusSqr;
    float ViewBrightness;
    float ColorGradingAdaptationSpeed;

    // Procedural color grading
    Float4 uTemperatureScale;
    Float4 uTemperatureStrength;
    Float4 uGrain;
    Float4 uGamma;
    Float4 uLift;
    Float4 uPresaturation;
    Float4 uLuminanceNormalization;

    uint64_t PrefilteredMapSampler;
    uint64_t IrradianceMapSampler;

    int32_t NumDirectionalLights;
    int32_t Padding4;
    int32_t Padding5;
    int32_t DebugMode;

    Float4 LightDirs[MAX_DIRECTIONAL_LIGHTS];            // Direction, W-channel is not used
    Float4 LightColors[MAX_DIRECTIONAL_LIGHTS];          // RGB, alpha - ambient intensity
    uint32_t LightParameters[MAX_DIRECTIONAL_LIGHTS][4];      // RenderMask, FirstCascade, NumCascades, W-channel is not used
};

struct SInstanceUniformBuffer {
    Float4x4 TransformMatrix;
    Float3x4 ModelNormalToViewSpace;
    Float4 LightmapOffset;
    Float4 uaddr_0;
    Float4 uaddr_1;
    Float4 uaddr_2;
    Float4 uaddr_3;
};

struct SShadowInstanceUniformBuffer {
    Float4x4 TransformMatrix; // TODO: 3x4
                              // For material with vertex deformations:
    Float4 uaddr_0;
    Float4 uaddr_1;
    Float4 uaddr_2;
    Float4 uaddr_3;
};

class AFrameResources {
public:
    void Initialize();
    void Deinitialize();

    void UploadUniforms();

    // Contains constant data for single draw call.
    // Don't use to store long-live data.
    std::unique_ptr< ACircularBuffer > ConstantBuffer;

    // Contains constant data for single frame.
    // Use to store data actual during one frame.
    std::unique_ptr< AFrameConstantBuffer > FrameConstantBuffer;

    GHI::Texture IrradianceMap;
    GHI::Sampler IrradianceMapSampler;
    GHI::BindlessSampler IrradianceMapBindless;

    GHI::Texture PrefilteredMap;
    GHI::Sampler PrefilteredMapSampler;
    GHI::BindlessSampler PrefilteredMapBindless;

    GHI::Texture ClusterLookup;
    GHI::Texture ClusterItemTBO;
    GHI::Buffer  ClusterItemBuffer;
    GHI::Buffer  Saq;

    GHI::ShaderResources        Resources;
    GHI::ShaderBufferBinding    BufferBinding[6];
    GHI::ShaderBufferBinding *  ViewUniformBufferBinding;
    GHI::ShaderBufferBinding *  InstanceUniformBufferBinding;
    GHI::ShaderBufferBinding *  SkeletonBufferBinding;
    GHI::ShaderBufferBinding *  CascadeBufferBinding;
    GHI::ShaderBufferBinding *  LightBufferBinding;
    GHI::ShaderBufferBinding *  IBLBufferBinding;
    GHI::ShaderTextureBinding   TextureBindings[16];
    GHI::ShaderSamplerBinding   SamplerBindings[16];

private:
    void SetViewUniforms();
};

AN_FORCEINLINE void StoreFloat3x3AsFloat3x4Transposed( Float3x3 const & _In, Float3x4 & _Out ) {
    _Out[0][0] = _In[0][0];
    _Out[0][1] = _In[1][0];
    _Out[0][2] = _In[2][0];
    _Out[0][3] = 0;

    _Out[1][0] = _In[0][1];
    _Out[1][1] = _In[1][1];
    _Out[1][2] = _In[2][1];
    _Out[1][3] = 0;

    _Out[2][0] = _In[0][2];
    _Out[2][1] = _In[1][2];
    _Out[2][2] = _In[2][2];
    _Out[2][3] = 0;
}

AN_FORCEINLINE void StoreFloat3x4AsFloat4x4Transposed( Float3x4 const & _In, Float4x4 & _Out ) {
    _Out[0][0] = _In[0][0];
    _Out[0][1] = _In[1][0];
    _Out[0][2] = _In[2][0];
    _Out[0][3] = 0;

    _Out[1][0] = _In[0][1];
    _Out[1][1] = _In[1][1];
    _Out[1][2] = _In[2][1];
    _Out[1][3] = 0;

    _Out[2][0] = _In[0][2];
    _Out[2][1] = _In[1][2];
    _Out[2][2] = _In[2][2];
    _Out[2][3] = 0;

    _Out[3][0] = _In[0][3];
    _Out[3][1] = _In[1][3];
    _Out[3][2] = _In[2][3];
    _Out[3][3] = 1;
}

// Globals
extern GHI::Device          GDevice;
extern GHI::State           GState;
extern GHI::CommandBuffer   Cmd;
extern SRenderFrame *       GFrameData;
extern SRenderView *        GRenderView;
extern ARenderArea          GRenderViewArea;
extern AShaderSources       GShaderSources;
extern AFrameResources      GFrameResources;

}
