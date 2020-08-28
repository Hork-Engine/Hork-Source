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

#include <RenderCore/FrameGraph/FrameGraph.h>
#include <RenderCore/Sampler.h>

#include <Runtime/Public/RenderCore.h>
#include <Runtime/Public/RuntimeVariable.h>

#include <Core/Public/Logger.h>


#include "CircularBuffer.h"
#include "FrameConstantBuffer.h"

#define SCISSOR_TEST false
#define DEPTH_PREPASS

#define SHADOWMAP_PCF
//#define SHADOWMAP_PCSS
//#define SHADOWMAP_VSM
//#define SHADOWMAP_EVSM

extern ARuntimeVariable RVRenderSnapshot;

extern TRef< RenderCore::IDevice > GDevice;

extern ARuntimeVariable RVMotionBlur;
extern ARuntimeVariable RVSSLR;
extern ARuntimeVariable RVSSAO;

AN_FORCEINLINE RenderCore::IBuffer * GPUBufferHandle( ABufferGPU * _Buffer )
{ 
    return _Buffer->pBuffer;
}

AN_FORCEINLINE RenderCore::ITexture * GPUTextureHandle( ATextureGPU * _Texture )
{
    return _Texture->pTexture;
}

RenderCore::STextureResolution2D GetFrameResoultion();

void DrawSAQ( RenderCore::IPipeline * Pipeline );

void BindVertexAndIndexBuffers( SRenderInstance const * Instance );

void BindVertexAndIndexBuffers( SShadowRenderInstance const * Instance );

void BindVertexAndIndexBuffers( SLightPortalRenderInstance const * Instance );

void BindSkeleton( size_t _Offset, size_t _Size );
void BindSkeletonMotionBlur( size_t _Offset, size_t _Size );

void BindTextures( SMaterialFrameData * MaterialInstance );

void SetInstanceUniforms( SRenderInstance const * Instance );

void SetInstanceUniformsFB( SRenderInstance const * Instance );

void SetShadowInstanceUniforms( SShadowRenderInstance const * Instance );

void * SetDrawCallUniforms( size_t SizeInBytes );

template< typename T >
T * SetDrawCallUniforms() {
    return (T *)SetDrawCallUniforms( sizeof( T ) );
}

void SaveSnapshot( RenderCore::ITexture & _Texture );

AString LoadShader( const char * FileName, SMaterialShader const * Predefined = nullptr );
AString LoadShaderFromString( const char * FileName, const char * Source, SMaterialShader const * Predefined = nullptr );

void CreateFullscreenQuadPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * VertexShader, const char * FragmentShader, RenderCore::BLENDING_PRESET BlendingPreset = RenderCore::BLENDING_NO_BLEND );

void CreateFullscreenQuadPipelineGS( TRef< RenderCore::IPipeline > * ppPipeline, const char * VertexShader, const char * FragmentShader, const char * GeometryShader, RenderCore::BLENDING_PRESET BlendingPreset = RenderCore::BLENDING_NO_BLEND );

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

    void Build( RenderCore::SHADER_TYPE _ShaderType, TRef< RenderCore::IShaderModule > & _Module ) {
        using namespace RenderCore;

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
        predefines += "#define MAX_TOTAL_SHADOW_CASCADES_PER_VIEW " + Math::ToString( MAX_TOTAL_SHADOW_CASCADES_PER_VIEW ) + "\n";


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

        if ( RVSSLR ) {
            predefines += "#define WITH_SSLR\n";
        }

        if ( RVSSAO ) {
            predefines += "#define WITH_SSAO\n";
        }

        Sources[0] = "#version 450\n"
            "#extension GL_ARB_bindless_texture : enable\n";
        Sources[1] = predefines.CStr();

        GDevice->CreateShaderFromCode( _ShaderType, NumSources, Sources, &Log, &_Module );

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

struct SViewUniformBuffer
{
    Float4x4 OrthoProjection;
    Float4x4 ViewProjection;
    Float4x4 ProjectionMatrix;
    Float4x4 InverseProjectionMatrix;
    Float4x4 InverseViewMatrix;

    // Reprojection from viewspace to previous frame projected coordinates:
    // ReprojectionMatrix = ProjectionMatrixPrevFrame * WorldspaceToViewspacePrevFrame * ViewspaceToWorldspace
    Float4x4 ReprojectionMatrix;

    // Reprojection from viewspace to previous frame viewspace coordinates:
    // ViewspaceReprojection = WorldspaceToViewspacePrevFrame * ViewspaceToWorldspace
    Float4x4 ViewspaceReprojection;

    Float3x4 WorldNormalToViewSpace;

    // ViewportParams
    Float2 InvViewportSize;
    float ZNear;
    float ZFar;

    Float4 ProjectionInfo;

    // Timers
    float GameRunningTimeSeconds;
    float GameplayTimeSeconds;

    float DynamicResolutionRatioX;
    float DynamicResolutionRatioY;

    Float2 FeedbackBufferResolutionRatio;
    Float2 VTPageCacheCapacity;
    Float4 VTPageTranslationOffsetAndScale;

    Float3 ViewPosition;
    float TimeDelta;

    Float4 PostprocessBloomMix;

    // Postprocess attribs
    float BloomEnabled;
    float ToneMappingExposure;
    float ColorGrading;
    float FXAA;

    Float4 VignetteColorIntensity;        // rgb, intensity
    float VignetteOuterRadiusSqr;
    float VignetteInnerRadiusSqr;
    float ViewBrightness;
    float ColorGradingAdaptationSpeed;

    float SSLRSampleOffset;
    float SSLRMaxDist;
    float IsPerspective;
    float TessellationLevel;

    uint64_t PrefilteredMapSampler;
    uint64_t IrradianceMapSampler;

    int32_t NumDirectionalLights;
    int32_t Pad3;
    int32_t Pad4;
    int32_t DebugMode;

    Float4 LightDirs[MAX_DIRECTIONAL_LIGHTS];            // Direction, W-channel is not used
    Float4 LightColors[MAX_DIRECTIONAL_LIGHTS];          // RGB, alpha - ambient intensity
    uint32_t LightParameters[MAX_DIRECTIONAL_LIGHTS][4];      // RenderMask, FirstCascade, NumCascades, W-channel is not used
};

static_assert( sizeof( SViewUniformBuffer ) <= ( 16<<10 ), "sizeof SViewUniformBuffer > 16 kB" );

struct SInstanceUniformBuffer
{
    Float4x4 TransformMatrix;
    Float4x4 TransformMatrixP;
    Float3x4 ModelNormalToViewSpace;
    Float4 LightmapOffset;
    Float4 uaddr_0;
    Float4 uaddr_1;
    Float4 uaddr_2;
    Float4 uaddr_3;
    Float2 VTOffset;
    Float2 VTScale;
    uint32_t VTUnit;
    uint32_t Pad0;
    uint32_t Pad1;
    uint32_t Pad2;
};

struct SFeedbackUniformBuffer
{
    Float4x4 TransformMatrix; // Instance MVP
    Float2 VTOffset;
    Float2 VTScale;
    uint32_t VTUnit;
    uint32_t Pad[3];
};

struct SShadowInstanceUniformBuffer
{
    Float4x4 TransformMatrix; // TODO: 3x4
    //Float4 ModelNormalToViewSpace0;
    //Float4 ModelNormalToViewSpace1;
    //Float4 ModelNormalToViewSpace2;
    Float4 uaddr_0;
    Float4 uaddr_1;
    Float4 uaddr_2;
    Float4 uaddr_3;
    uint32_t CascadeMask;
    uint32_t Pad[3];
};

class AFrameResources {
public:
    void Initialize();
    void Deinitialize();

    void UploadUniforms();

    void SetShadowMatrixBinding();
    void SetShadowCascadeBinding( int FirstCascade, int NumCascades );

    // Contains constant data for single draw call.
    // Don't use to store long-live data.
    TRef< ACircularBuffer > ConstantBuffer;

    // Contains constant data for single frame.
    // Use to store data actual during one frame.
    TRef< AFrameConstantBuffer > FrameConstantBuffer;

    TRef< RenderCore::ITexture > WhiteTexture;

    TRef< RenderCore::ITexture > IrradianceMap;
    RenderCore::Sampler IrradianceMapSampler;
    TRef< RenderCore::IBindlessSampler > IrradianceMapBindless;

    TRef< RenderCore::ITexture > PrefilteredMap;
    RenderCore::Sampler PrefilteredMapSampler;
    TRef< RenderCore::IBindlessSampler > PrefilteredMapBindless;

    TRef< RenderCore::ITexture > ClusterLookup;
    TRef< RenderCore::IBufferView > ClusterItemTBO;
    TRef< RenderCore::IBuffer > ClusterItemBuffer;
    TRef< RenderCore::IBuffer > Saq;

    RenderCore::SShaderResources        Resources;
    RenderCore::SShaderBufferBinding    BufferBinding[8];
    RenderCore::SShaderBufferBinding *  ViewUniformBufferBinding;       // binding 0
    RenderCore::SShaderBufferBinding *  DrawCallUniformBufferBinding;   // binding 1
    RenderCore::SShaderBufferBinding *  SkeletonBufferBinding;          // binding 2
    RenderCore::SShaderBufferBinding    ShadowCascadeBinding;           // binding 3
    RenderCore::SShaderBufferBinding    ShadowMatrixBinding;            // binding 3
    RenderCore::SShaderBufferBinding *  LightBufferBinding;             // binding 4
    RenderCore::SShaderBufferBinding *  IBLBufferBinding;               // binding 5
    RenderCore::SShaderBufferBinding *  VTBufferBinding;                // binding 6
    RenderCore::SShaderBufferBinding *  SkeletonBufferBindingMB;        // binding 7
    RenderCore::SShaderTextureBinding   TextureBindings[RenderCore::MAX_SAMPLER_SLOTS];
    RenderCore::SShaderSamplerBinding   SamplerBindings[RenderCore::MAX_SAMPLER_SLOTS];

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
extern RenderCore::IImmediateContext * rcmd;
extern SRenderFrame *       GFrameData;
extern SRenderView *        GRenderView;
extern ARenderArea          GRenderViewArea;
extern AShaderSources       GShaderSources;
extern AFrameResources      GFrameResources;
