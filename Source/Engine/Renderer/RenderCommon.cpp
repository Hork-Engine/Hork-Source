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

#include "RenderCommon.h"

#include "RenderBackend.h"

#include "IrradianceGenerator.h"
#include "EnvProbeGenerator.h"
#include "CubemapGenerator.h"
#include "AtmosphereRenderer.h"

#include "VT/VirtualTextureFeedback.h"
#include "VT/VirtualTexture.h"

#include <Core/Public/Color.h>
#include <Core/Public/Image.h>
#include <Core/Public/CriticalError.h>
#include <Core/Public/Logger.h>

ARuntimeVariable RVDebugRenderMode( _CTS( "DebugRenderMode" ), _CTS( "0" ), VAR_CHEAT );
ARuntimeVariable RVPostprocessBloomScale( _CTS( "PostprocessBloomScale" ), _CTS( "1" ) );
ARuntimeVariable RVPostprocessBloom( _CTS( "PostprocessBloom" ), _CTS( "1" ) );
ARuntimeVariable RVPostprocessBloomParam0( _CTS( "PostprocessBloomParam0" ), _CTS( "0.5" ) );
ARuntimeVariable RVPostprocessBloomParam1( _CTS( "PostprocessBloomParam1" ), _CTS( "0.3" ) );
ARuntimeVariable RVPostprocessBloomParam2( _CTS( "PostprocessBloomParam2" ), _CTS( "0.04" ) );
ARuntimeVariable RVPostprocessBloomParam3( _CTS( "PostprocessBloomParam3" ), _CTS( "0.01" ) );
//ARuntimeVariable RVPostprocessToneExposure( _CTS( "PostprocessToneExposure" ), _CTS( "0.05" ) );
ARuntimeVariable RVPostprocessToneExposure( _CTS( "PostprocessToneExposure" ), _CTS( "0.4" ) );
ARuntimeVariable RVBrightness( _CTS( "Brightness" ), _CTS( "1" ) );
ARuntimeVariable RVSSLRSampleOffset( _CTS( "SSLRSampleOffset" ), _CTS( "0.1" ) );
ARuntimeVariable RVSSLRMaxDist( _CTS( "SSLRMaxDist" ), _CTS( "10" ) );
ARuntimeVariable RVTessellationLevel( _CTS( "TessellationLevel" ), _CTS( "0.05" ) );
ARuntimeVariable RVMotionBlur( _CTS( "MotionBlur" ), _CTS( "1" ) );
ARuntimeVariable RVSSLR( _CTS( "SSLR" ), _CTS( "1" ) );
ARuntimeVariable RVSSAO( _CTS( "SSAO" ), _CTS( "1" ) );

extern ARuntimeVariable RVFxaa;

TRef< RenderCore::IDevice > GDevice;
RenderCore::IImmediateContext * rcmd;
SRenderFrame *       GFrameData;
SRenderView *        GRenderView;
ARenderArea          GRenderViewArea;
AShaderSources       GShaderSources;
AFrameResources      GFrameResources;

RenderCore::STextureResolution2D GetFrameResoultion()
{
    return RenderCore::STextureResolution2D( GFrameData->AllocSurfaceWidth, GFrameData->AllocSurfaceHeight );
}

void DrawSAQ( RenderCore::IPipeline * Pipeline, unsigned int InstanceCount )
{
    const RenderCore::SDrawCmd drawCmd = { 4, InstanceCount, 0, 0 };
    rcmd->BindPipeline( Pipeline );
    rcmd->BindVertexBuffer( 0, GFrameResources.Saq, 0 );
    rcmd->BindIndexBuffer( NULL, RenderCore::INDEX_TYPE_UINT16, 0 );
    rcmd->Draw( &drawCmd );
}

void DrawSphere( RenderCore::IPipeline * Pipeline, unsigned int InstanceCount )
{
    RenderCore::SDrawIndexedCmd drawCmd = {};
    drawCmd.IndexCountPerInstance = GFrameResources.SphereMesh->IndexCount;
    drawCmd.InstanceCount = InstanceCount;

    rcmd->BindPipeline( Pipeline );
    rcmd->BindVertexBuffer( 0, GFrameResources.SphereMesh->VertexBuffer );
    rcmd->BindIndexBuffer( GFrameResources.SphereMesh->IndexBuffer, RenderCore::INDEX_TYPE_UINT16 );
    rcmd->Draw( &drawCmd );
}

void BindTextures( SMaterialFrameData * Instance, int MaxTextures )
{
    AN_ASSERT( Instance );

    ATextureGPU ** texture = Instance->Textures;

    int n = Math::Min( Instance->NumTextures, MaxTextures );

    for ( int t = 0 ; t < n ; t++, texture++ ) {
        GFrameResources.TextureBindings[t]->pTexture = *texture ? GPUTextureHandle( *texture ) : nullptr;
    }
}

void BindVertexAndIndexBuffers( SRenderInstance const * _Instance ) {
    RenderCore::IBuffer * pVertexBuffer = GPUBufferHandle( _Instance->VertexBuffer );
    RenderCore::IBuffer * pIndexBuffer = GPUBufferHandle( _Instance->IndexBuffer );

    AN_ASSERT( pVertexBuffer );
    AN_ASSERT( pIndexBuffer );

    rcmd->BindVertexBuffer( 0, pVertexBuffer, _Instance->VertexBufferOffset );
    rcmd->BindIndexBuffer( pIndexBuffer, RenderCore::INDEX_TYPE_UINT32, _Instance->IndexBufferOffset );
}

void BindVertexAndIndexBuffers( SShadowRenderInstance const * _Instance ) {
    RenderCore::IBuffer * pVertexBuffer = GPUBufferHandle( _Instance->VertexBuffer );
    RenderCore::IBuffer * pIndexBuffer = GPUBufferHandle( _Instance->IndexBuffer );

    AN_ASSERT( pVertexBuffer );
    AN_ASSERT( pIndexBuffer );

    rcmd->BindVertexBuffer( 0, pVertexBuffer, _Instance->VertexBufferOffset );
    rcmd->BindIndexBuffer( pIndexBuffer, RenderCore::INDEX_TYPE_UINT32, _Instance->IndexBufferOffset );
}

void BindVertexAndIndexBuffers( SLightPortalRenderInstance const * _Instance ) {
    RenderCore::IBuffer * pVertexBuffer = GPUBufferHandle( _Instance->VertexBuffer );
    RenderCore::IBuffer * pIndexBuffer = GPUBufferHandle( _Instance->IndexBuffer );

    AN_ASSERT( pVertexBuffer );
    AN_ASSERT( pIndexBuffer );

    rcmd->BindVertexBuffer( 0, pVertexBuffer, _Instance->VertexBufferOffset );
    rcmd->BindIndexBuffer( pIndexBuffer, RenderCore::INDEX_TYPE_UINT32, _Instance->IndexBufferOffset );
}

void BindSkeleton( size_t _Offset, size_t _Size ) {
    GFrameResources.SkeletonBufferBinding->BindingOffset = _Offset;
    GFrameResources.SkeletonBufferBinding->BindingSize = _Size;
}

void BindSkeletonMotionBlur( size_t _Offset, size_t _Size ) {
    GFrameResources.SkeletonBufferBindingMB->BindingOffset = _Offset;
    GFrameResources.SkeletonBufferBindingMB->BindingSize = _Size;
}

void SetInstanceUniforms( SRenderInstance const * Instance ) {
    size_t offset = GFrameResources.ConstantBuffer->Allocate( sizeof( SInstanceUniformBuffer ) );

    SInstanceUniformBuffer * pUniformBuf = reinterpret_cast< SInstanceUniformBuffer * >(GFrameResources.ConstantBuffer->GetMappedMemory() + offset);

    Core::Memcpy( &pUniformBuf->TransformMatrix, &Instance->Matrix, sizeof( pUniformBuf->TransformMatrix ) );
    Core::Memcpy( &pUniformBuf->TransformMatrixP, &Instance->MatrixP, sizeof( pUniformBuf->TransformMatrixP ) );
    StoreFloat3x3AsFloat3x4Transposed( Instance->ModelNormalToViewSpace, pUniformBuf->ModelNormalToViewSpace );
    Core::Memcpy( &pUniformBuf->LightmapOffset, &Instance->LightmapOffset, sizeof( pUniformBuf->LightmapOffset ) );
    Core::Memcpy( &pUniformBuf->uaddr_0, Instance->MaterialInstance->UniformVectors, sizeof( Float4 )*Instance->MaterialInstance->NumUniformVectors );

    // TODO:
    pUniformBuf->VTOffset = Float2( 0.0f );//Instance->VTOffset;
    pUniformBuf->VTScale = Float2( 1.0f );//Instance->VTScale;
    pUniformBuf->VTUnit = 0;//Instance->VTUnit;

    GFrameResources.DrawCallUniformBufferBinding->BindingOffset = offset;
    GFrameResources.DrawCallUniformBufferBinding->BindingSize = sizeof( SInstanceUniformBuffer );
}

void SetInstanceUniformsFB( SRenderInstance const * Instance ) {
    size_t offset = GFrameResources.ConstantBuffer->Allocate( sizeof( SFeedbackUniformBuffer ) );

    SFeedbackUniformBuffer * pUniformBuf = reinterpret_cast< SFeedbackUniformBuffer * >(GFrameResources.ConstantBuffer->GetMappedMemory() + offset);

    Core::Memcpy( &pUniformBuf->TransformMatrix, &Instance->Matrix, sizeof( pUniformBuf->TransformMatrix ) );

    // TODO:
    pUniformBuf->VTOffset = Float2(0.0f);//Instance->VTOffset;
    pUniformBuf->VTScale = Float2(1.0f);//Instance->VTScale;
    pUniformBuf->VTUnit = 0;//Instance->VTUnit;

    GFrameResources.DrawCallUniformBufferBinding->BindingOffset = offset;
    GFrameResources.DrawCallUniformBufferBinding->BindingSize = sizeof( SFeedbackUniformBuffer );
}

void SetShadowInstanceUniforms( SShadowRenderInstance const * Instance ) {
    size_t offset = GFrameResources.ConstantBuffer->Allocate( sizeof( SShadowInstanceUniformBuffer ) );

    SShadowInstanceUniformBuffer * pUniformBuf = reinterpret_cast< SShadowInstanceUniformBuffer * >(GFrameResources.ConstantBuffer->GetMappedMemory() + offset);

    StoreFloat3x4AsFloat4x4Transposed( Instance->WorldTransformMatrix, pUniformBuf->TransformMatrix );

    if ( Instance->MaterialInstance ) {
        Core::Memcpy( &pUniformBuf->uaddr_0, Instance->MaterialInstance->UniformVectors, sizeof( Float4 )*Instance->MaterialInstance->NumUniformVectors );
    }

    pUniformBuf->CascadeMask = Instance->CascadeMask;

    GFrameResources.DrawCallUniformBufferBinding->BindingOffset = offset;
    GFrameResources.DrawCallUniformBufferBinding->BindingSize = sizeof( SShadowInstanceUniformBuffer );
}

void * SetDrawCallUniforms( size_t SizeInBytes ) {
    size_t offset = GFrameResources.ConstantBuffer->Allocate( SizeInBytes );

#if 0
    GFrameResources.DrawCallUniformBufferBindingPostProcess->BindingOffset = offset;
    GFrameResources.DrawCallUniformBufferBindingPostProcess->BindingSize = SizeInBytes;
#else
    GFrameResources.DrawCallUniformBufferBinding->BindingOffset = offset;
    GFrameResources.DrawCallUniformBufferBinding->BindingSize = SizeInBytes;
#endif

    return GFrameResources.ConstantBuffer->GetMappedMemory() + offset;
}

void CreateFullscreenQuadPipeline( TRef< RenderCore::IPipeline > * ppPipeline, const char * VertexShader, const char * FragmentShader, RenderCore::SSamplerInfo * Samplers, int NumSamplers, RenderCore::BLENDING_PRESET BlendingPreset ) {
    using namespace RenderCore;

    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;
    rsd.bScissorEnable = false;

    SBlendingStateInfo & bsd = pipelineCI.BS;

    if ( BlendingPreset != BLENDING_NO_BLEND ) {
        bsd.RenderTargetSlots[0].SetBlendingPreset( BlendingPreset );
    }

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            0
        }
    };

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    TRef< IShaderModule > vertexShaderModule, fragmentShaderModule;

    AString vertexSourceCode = LoadShader( VertexShader );
    AString fragmentSourceCode = LoadShader( FragmentShader );

    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSourceCode.CStr() );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( fragmentSourceCode.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float2 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.SS.NumSamplers = NumSamplers;
    pipelineCI.SS.Samplers = Samplers;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateFullscreenQuadPipelineGS( TRef< RenderCore::IPipeline > * ppPipeline, const char * VertexShader, const char * FragmentShader, const char * GeometryShader, RenderCore::SSamplerInfo * Samplers, int NumSamplers, RenderCore::BLENDING_PRESET BlendingPreset ) {
    using namespace RenderCore;

    SPipelineCreateInfo pipelineCI;

    SRasterizerStateInfo & rsd = pipelineCI.RS;
    rsd.CullMode = POLYGON_CULL_FRONT;
    rsd.bScissorEnable = false;

    SBlendingStateInfo & bsd = pipelineCI.BS;

    if ( BlendingPreset != BLENDING_NO_BLEND ) {
        bsd.RenderTargetSlots[0].SetBlendingPreset( BlendingPreset );
    }

    SDepthStencilStateInfo & dssd = pipelineCI.DSS;
    dssd.bDepthEnable = false;
    dssd.DepthWriteMask = DEPTH_WRITE_DISABLE;

    static const SVertexAttribInfo vertexAttribs[] = {
        {
            "InPosition",
            0,              // location
            0,              // buffer input slot
            VAT_FLOAT2,
            VAM_FLOAT,
            0,              // InstanceDataStepRate
            0
        }
    };

    AString vertexAttribsShaderString = ShaderStringForVertexAttribs< AString >( vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ) );

    TRef< IShaderModule > vertexShaderModule, fragmentShaderModule, geometryShaderModule;

    AString vertexSourceCode = LoadShader( VertexShader );
    AString fragmentSourceCode = LoadShader( FragmentShader );
    AString geometrySourceCode = LoadShader( GeometryShader );

    GShaderSources.Clear();
    GShaderSources.Add( vertexAttribsShaderString.CStr() );
    GShaderSources.Add( vertexSourceCode.CStr() );
    GShaderSources.Build( VERTEX_SHADER, vertexShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( fragmentSourceCode.CStr() );
    GShaderSources.Build( FRAGMENT_SHADER, fragmentShaderModule );

    GShaderSources.Clear();
    GShaderSources.Add( geometrySourceCode.CStr() );
    GShaderSources.Build( GEOMETRY_SHADER, geometryShaderModule );

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;
    inputAssembly.bPrimitiveRestart = false;

    pipelineCI.pVS = vertexShaderModule;
    pipelineCI.pGS = geometryShaderModule;
    pipelineCI.pFS = fragmentShaderModule;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float2 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    pipelineCI.SS.NumSamplers = NumSamplers;
    pipelineCI.SS.Samplers = Samplers;

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void SaveSnapshot( RenderCore::ITexture & _Texture ) {

    const int w = _Texture.GetWidth();
    const int h = _Texture.GetHeight();
    const int numchannels = 3;
    const int size = w * h * numchannels;

    int hunkMark = GHunkMemory.SetHunkMark();

    byte * data = (byte *)GHunkMemory.Alloc( size );

#if 0
    _Texture.Read( 0, RenderCore::PIXEL_FORMAT_BYTE_RGB, size, 1, data );
#else
    float * fdata = (float *)GHunkMemory.Alloc( size*sizeof(float) );
    _Texture.Read( 0, RenderCore::FORMAT_FLOAT3, size*sizeof(float), 1, fdata );
    // to sRGB
    for ( int i = 0 ; i < size ; i++ ) {
        data[i] = LinearToSRGB_UChar( fdata[i] );
    }
#endif

    FlipImageY( data, w, h, numchannels, w * numchannels );
    
    static int n = 0;
    AFileStream f;
    if ( f.OpenWrite( Core::Fmt( "snapshots/%d.png", n++ ) ) ) {
         WritePNG( f, w, h, numchannels, data, w*numchannels );
    }

    GHunkMemory.ClearToMark( hunkMark );
}

struct SIncludeCtx {
    /** Callback for file loading */
    bool ( *LoadFile )( const char * FileName, AString & Source );

    /** Root path for includes */
    const char * PathToIncludes;

    /** Predefined shaders */
    SMaterialShader const * Predefined;
};

// Modified version of stb_include.h v0.02 originally written by Sean Barrett and Michal Klos

struct SIncludeInfo
{
    int offset;
    int end;
    const char *filename;
    int len;
    int next_line_after;
    SIncludeInfo * next;
};

static void AddInclude( SIncludeInfo *& list, SIncludeInfo *& prev, int offset, int end, const char *filename, int len, int next_line ) {
    SIncludeInfo *z = (SIncludeInfo *)GZoneMemory.Alloc( sizeof( SIncludeInfo ) );
    z->offset = offset;
    z->end = end;
    z->filename = filename;
    z->len = len;
    z->next_line_after = next_line;
    z->next = NULL;
    if ( prev ) {
        prev->next = z;
        prev = z;
    } else {
        list = prev = z;
    }
}

static void FreeIncludes( SIncludeInfo * list ) {
    SIncludeInfo * next;
    for ( SIncludeInfo * includeInfo = list ; includeInfo ; includeInfo = next ) {
        next = includeInfo->next;
        GZoneMemory.Free( includeInfo );
    }
}

static AN_FORCEINLINE int IsSpace( int ch ) {
    return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

// find location of all #include
static SIncludeInfo * FindIncludes( const char * text ) {
    int line_count = 1;
    const char *s = text, *start;
    SIncludeInfo *list = NULL, *prev = NULL;
    while ( *s ) {
        // parse is always at start of line when we reach here
        start = s;
        while ( *s == ' ' || *s == '\t' ) {
            ++s;
        }
        if ( *s == '#' ) {
            ++s;
            while ( *s == ' ' || *s == '\t' )
                ++s;
            if ( !Core::StrcmpN( s, "include", 7 ) && IsSpace( s[7] ) ) {
                s += 7;
                while ( *s == ' ' || *s == '\t' )
                    ++s;
                if ( *s == '"' ) {
                    const char *t = ++s;
                    while ( *t != '"' && *t != '\n' && *t != '\r' && *t != 0 )
                        ++t;
                    if ( *t == '"' ) {
                        int len = t - s;
                        const char * filename = s;
                        s = t;
                        while ( *s != '\r' && *s != '\n' && *s != 0 )
                            ++s;
                        // s points to the newline, so s-start is everything except the newline
                        AddInclude( list, prev, start-text, s-text, filename, len, line_count+1 );
                    }
                }
            }
        }
        while ( *s != '\r' && *s != '\n' && *s != 0 )
            ++s;
        if ( *s == '\r' || *s == '\n' ) {
            s = s + (s[0] + s[1] == '\r' + '\n' ? 2 : 1);
        }
        ++line_count;
    }
    return list;
}

static void CleanComments( char * s ) {
start:
    while ( *s ) {
        if ( *s == '/' ) {
            if ( *(s+1) == '/' ) {
                *s++ = ' ';
                *s++ = ' ';
                while ( *s && *s != '\n' )
                    *s++ = ' ';
                continue;
            }
            if ( *(s+1) == '*' ) {
                *s++ = ' ';
                *s++ = ' ';
                while ( *s ) {
                    if ( *s == '*' && *(s+1) == '/' ) {
                        *s++ = ' ';
                        *s++ = ' ';
                        goto start;
                    }
                    if ( *s != '\n' ) {
                        *s++ = ' ';
                    } else {
                        s++;
                    }
                }
                // end of file inside comment
                return;
            }
        }
        s++;
    }
}

static bool LoadShaderWithInclude( SIncludeCtx * Ctx, const char * FileName, AString & Out );

static bool LoadShaderFromString( SIncludeCtx * Ctx, const char * FileName, AString const & Source, AString & Out ) {
    char temp[4096];
    SIncludeInfo * includeList = FindIncludes( Source.CStr() );
    size_t sourceOffset = 0;

    for ( SIncludeInfo * includeInfo = includeList ; includeInfo ; includeInfo = includeInfo->next ) {
        Out.ConcatN( &Source[sourceOffset], includeInfo->offset - sourceOffset );

        if ( Ctx->Predefined && includeInfo->filename[0] == '$' ) {
            // predefined source
            Out.Concat( "#line 1 \"" );
            Out.ConcatN( includeInfo->filename, includeInfo->len );
            Out.Concat( "\"\n" );

            SMaterialShader const * s;
            for ( s = Ctx->Predefined ; s ; s = s->Next ) {
                if ( !Core::StricmpN( s->SourceName, includeInfo->filename, includeInfo->len ) ) {
                    break;
                }
            }

            if ( !s || !LoadShaderFromString( Ctx, FileName, s->Code, Out ) ) {
                FreeIncludes( includeList );
                return false;
            }
        } else {
            Out.Concat( "#line 1 \"" );
            Out.Concat( Ctx->PathToIncludes );
            Out.ConcatN( includeInfo->filename, includeInfo->len );
            Out.Concat( "\"\n" );

            Core::Strcpy( temp, sizeof( temp ), Ctx->PathToIncludes );
            Core::StrcatN( temp, sizeof( temp ), includeInfo->filename, includeInfo->len );
            if ( !LoadShaderWithInclude( Ctx, temp, Out ) ) {
                FreeIncludes( includeList );
                return false;
            }
        }        

        Core::Sprintf( temp, sizeof( temp ), "\n#line %d \"%s\"", includeInfo->next_line_after, FileName ? FileName : "source-file" );
        Out.Concat( temp );

        sourceOffset = includeInfo->end;
    }

    Out.ConcatN( &Source[sourceOffset], Source.Length() - sourceOffset + 1 );
    FreeIncludes( includeList );

    return true;
}

static bool LoadShaderWithInclude( SIncludeCtx * Ctx, const char * FileName, AString & Out ) {
    AString source;

    if ( !Ctx->LoadFile( FileName, source ) ) {
        GLogger.Printf( "Couldn't load %s\n", FileName );
        return false;
    }

    CleanComments( source.ToPtr() );

    return LoadShaderFromString( Ctx, FileName, source, Out );
}

static bool GetShaderSource( const char * FileName, AString & Source ) {
    AFileStream f;
    if ( !f.OpenRead( FileName ) ) {
        return false;
    }
    Source.FromFile( f );
    return true;
}

AString LoadShader( const char * FileName, SMaterialShader const * Predefined ) {
    AString path = __FILE__;
    path.StripFilename();
    path.FixPath();
    path += "/Shaders/";

    SIncludeCtx ctx;
    ctx.LoadFile = GetShaderSource;
    ctx.PathToIncludes = path.CStr();
    ctx.Predefined = Predefined;

    AString result;
    result.Concat( Core::Fmt( "#line 1 \"%s\"\n", FileName ) );

    if ( !LoadShaderWithInclude( &ctx, (path + FileName).CStr(), result ) ) {
        CriticalError( "LoadShader: failed to open %s\n", FileName );
    }

    return result;
}

AString LoadShaderFromString( const char * FileName, const char * Source, SMaterialShader const * Predefined ) {
    AString path = __FILE__;
    path.StripFilename();
    path.FixPath();
    path += "/Shaders/";

    SIncludeCtx ctx;
    ctx.LoadFile = GetShaderSource;
    ctx.PathToIncludes = path.CStr();
    ctx.Predefined = Predefined;

    AString result;
    result.Concat( Core::Fmt( "#line 1 \"%s\"\n", FileName ) );

    AString source = Source;

    CleanComments( source.ToPtr() );

    if ( !LoadShaderFromString( &ctx, (path + FileName).CStr(), source, result ) ) {
        CriticalError( "LoadShader: failed to open %s\n", FileName );
    }

    return result;
}

void AFrameResources::Initialize() {
    ConstantBuffer = MakeRef< ACircularBuffer >( 2 * 1024 * 1024 ); // 2MB
    FrameConstantBuffer = MakeRef< AFrameConstantBuffer >( 2 * 1024 * 1024 ); // 2MB

    // Create sphere mesh for cubemap rendering
    SphereMesh = MakeRef< ASphereMesh >();

    // Create screen aligned quad
    {
        constexpr Float2 saqVertices[4] = {
            { Float2( -1.0f,  1.0f ) },
            { Float2( 1.0f,  1.0f ) },
            { Float2( -1.0f, -1.0f ) },
            { Float2( 1.0f, -1.0f ) }
        };

        RenderCore::SBufferCreateInfo bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.SizeInBytes = sizeof( saqVertices );
        GDevice->CreateBuffer( bufferCI, saqVertices, &Saq );
    }

    // Create white texture
    {
        GDevice->CreateTexture( RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_RGBA8, RenderCore::STextureResolution2D( 1, 1 ) ),
                                &WhiteTexture );
        RenderCore::STextureRect rect = {};
        rect.Dimension.X = 1;
        rect.Dimension.Y = 1;
        rect.Dimension.Z = 1;
        const byte data[4] = { 0xff, 0xff, 0xff, 0xff };
        WhiteTexture->WriteRect( rect, RenderCore::FORMAT_UBYTE4, sizeof( data ), 4, data );
    }

    // Create cluster lookup 3D texture
    GDevice->CreateTexture( RenderCore::MakeTexture( RenderCore::TEXTURE_FORMAT_RG32UI,
                                                     RenderCore::STextureResolution3D( MAX_FRUSTUM_CLUSTERS_X,
                                                                                       MAX_FRUSTUM_CLUSTERS_Y,
                                                                                       MAX_FRUSTUM_CLUSTERS_Z ) ), &ClusterLookup );
    // Create item buffer
    {
        // FIXME: Use SSBO?
        RenderCore::SBufferCreateInfo bufferCI = {};
        bufferCI.bImmutableStorage = true;
        bufferCI.ImmutableStorageFlags = RenderCore::IMMUTABLE_DYNAMIC_STORAGE;
        bufferCI.SizeInBytes = sizeof( SFrameLightData::ItemBuffer );
        GDevice->CreateBuffer( bufferCI, nullptr, &ClusterItemBuffer );

        RenderCore::SBufferViewCreateInfo bufferViewCI = {};
        bufferViewCI.Format = RenderCore::BUFFER_VIEW_PIXEL_FORMAT_R32UI;
        GDevice->CreateBufferView( bufferViewCI, ClusterItemBuffer, &ClusterItemTBO );
    }


#if 0
    ViewUniformBufferBindingPostProcess = PostProccessResources.AddBuffer( RenderCore::UNIFORM_BUFFER );
    ViewUniformBufferBindingPostProcess->pBuffer = FrameConstantBuffer->GetBuffer();

    DrawCallUniformBufferBindingPostProcess = PostProccessResources.AddBuffer( RenderCore::UNIFORM_BUFFER );
    DrawCallUniformBufferBindingPostProcess->pBuffer = ConstantBuffer->GetBuffer();
#endif

    ViewUniformBufferBinding = Resources.AddBuffer( RenderCore::UNIFORM_BUFFER );
    ViewUniformBufferBinding->pBuffer = FrameConstantBuffer->GetBuffer();

    DrawCallUniformBufferBinding = Resources.AddBuffer( RenderCore::UNIFORM_BUFFER );
    DrawCallUniformBufferBinding->pBuffer = ConstantBuffer->GetBuffer();

    SkeletonBufferBinding = Resources.AddBuffer( RenderCore::UNIFORM_BUFFER );
    SkeletonBufferBinding->pBuffer = nullptr;

    ShadowCascadeBinding = Resources.AddBuffer( RenderCore::UNIFORM_BUFFER );
    ShadowCascadeBinding->pBuffer = nullptr;

    LightBufferBinding = Resources.AddBuffer( RenderCore::UNIFORM_BUFFER );
    LightBufferBinding->pBuffer = FrameConstantBuffer->GetBuffer();

    IBLBufferBinding = Resources.AddBuffer( RenderCore::UNIFORM_BUFFER );
    IBLBufferBinding->pBuffer = FrameConstantBuffer->GetBuffer();

    VTBufferBinding = Resources.AddBuffer( RenderCore::UNIFORM_BUFFER );
    VTBufferBinding->pBuffer = FrameConstantBuffer->GetBuffer();

    SkeletonBufferBindingMB = Resources.AddBuffer( RenderCore::UNIFORM_BUFFER );
    SkeletonBufferBindingMB->pBuffer = nullptr;

    for ( int i = 0 ; i < RenderCore::MAX_SAMPLER_SLOTS ; i++ ) {
        TextureBindings[i] = Resources.AddTexture();
    }

    /////////////////////////////////////////////////////////////////////
    // test
    /////////////////////////////////////////////////////////////////////

    TRef< RenderCore::ITexture > skybox;
    {
        AAtmosphereRenderer atmosphereRenderer;
        atmosphereRenderer.Render( 512, Float3( -0.5f, -2, -10 ), &skybox );
#if 0
        RenderCore::STextureRect rect = {};
        rect.Dimension.X = rect.Dimension.Y = skybox->GetWidth();
        rect.Dimension.Z = 1;

        int hunkMark = GHunkMemory.SetHunkMark();
        float * data = (float *)GHunkMemory.Alloc( 512*512*3*sizeof( *data ) );
        //byte * ucdata = (byte *)GHunkMemory.Alloc( 512*512*3 );
        for ( int i = 0 ; i < 6 ; i++ ) {
            rect.Offset.Z = i;
            skybox->ReadRect( rect, RenderCore::FORMAT_FLOAT3, 512*512*3*sizeof( *data ), 1, data ); // TODO: check half float

            //FlipImageY( data, 512, 512, 3*sizeof(*data), 512*3*sizeof( *data ) );

            for ( int p = 0; p<512*512*3; p += 3 ) {
                StdSwap( data[p], data[p+2] );
                //ucdata[p] = Math::Clamp( data[p] * 255.0f, 0.0f, 255.0f );
                //ucdata[p+1] = Math::Clamp( data[p+1] * 255.0f, 0.0f, 255.0f );
                //ucdata[p+2] = Math::Clamp( data[p+2] * 255.0f, 0.0f, 255.0f );
            }

            AFileStream f;
            //f.OpenWrite( Core::Fmt( "skyface%d.png", i ) );
            //WritePNG( f, rect.Dimension.X, rect.Dimension.Y, 3, ucdata, rect.Dimension.X * 3 );

            f.OpenWrite( Core::Fmt( "xskyface%d.hdr", i ) );
            WriteHDR( f, rect.Dimension.X, rect.Dimension.Y, 3, data );
        }
        GHunkMemory.ClearToMark( hunkMark );
#endif
    }

#if 1
    TRef< RenderCore::ITexture > cubemap;
    TRef< RenderCore::ITexture > cubemap2;
    {
        const char * Cubemap[6] = {
            "ClearSky/rt.bmp",
            "ClearSky/lt.bmp",
            "ClearSky/up.bmp",
            "ClearSky/dn.bmp",
            "ClearSky/bk.bmp",
            "ClearSky/ft.bmp"
        };
        const char * Cubemap2[6] = {
            "DarkSky/rt.tga",
            "DarkSky/lt.tga",
            "DarkSky/up.tga",
            "DarkSky/dn.tga",
            "DarkSky/bk.tga",
            "DarkSky/ft.tga"
        };
        AImage rt, lt, up, dn, bk, ft;
        AImage const * cubeFaces[6] = { &rt,&lt,&up,&dn,&bk,&ft };
        rt.Load( Cubemap[0], nullptr, IMAGE_PF_BGR32F );
        lt.Load( Cubemap[1], nullptr, IMAGE_PF_BGR32F );
        up.Load( Cubemap[2], nullptr, IMAGE_PF_BGR32F );
        dn.Load( Cubemap[3], nullptr, IMAGE_PF_BGR32F );
        bk.Load( Cubemap[4], nullptr, IMAGE_PF_BGR32F );
        ft.Load( Cubemap[5], nullptr, IMAGE_PF_BGR32F );
#if 0
        const float HDRI_Scale = 4.0f;
        const float HDRI_Pow = 1.1f;
#else
        const float HDRI_Scale = 2;
        const float HDRI_Pow = 1;
#endif
        for ( int i = 0 ; i < 6 ; i++ ) {
            float * HDRI = (float*)cubeFaces[i]->GetData();
            int count = cubeFaces[i]->GetWidth()*cubeFaces[i]->GetHeight()*3;
            for ( int j = 0; j < count ; j += 3 ) {
                HDRI[j] = Math::Pow( HDRI[j + 0] * HDRI_Scale, HDRI_Pow );
                HDRI[j + 1] = Math::Pow( HDRI[j + 1] * HDRI_Scale, HDRI_Pow );
                HDRI[j + 2] = Math::Pow( HDRI[j + 2] * HDRI_Scale, HDRI_Pow );
            }
        }
        int w = cubeFaces[0]->GetWidth();
        RenderCore::STextureCreateInfo cubemapCI = {};
        cubemapCI.Type = RenderCore::TEXTURE_CUBE_MAP;
        cubemapCI.Format = RenderCore::TEXTURE_FORMAT_RGB32F;
        cubemapCI.Resolution.TexCubemap.Width = w;
        cubemapCI.NumLods = 1;
        GDevice->CreateTexture( cubemapCI, &cubemap );
        for ( int face = 0 ; face < 6 ; face++ ) {
            float * pSrc = (float *)cubeFaces[face]->GetData();

            RenderCore::STextureRect rect = {};
            rect.Offset.Z = face;
            rect.Dimension.X = w;
            rect.Dimension.Y = w;
            rect.Dimension.Z = 1;

            cubemap->WriteRect( rect, RenderCore::FORMAT_FLOAT3, w*w*3*sizeof( float ), 1, pSrc );
        }
        rt.Load( Cubemap2[0], nullptr, IMAGE_PF_BGR32F );
        lt.Load( Cubemap2[1], nullptr, IMAGE_PF_BGR32F );
        up.Load( Cubemap2[2], nullptr, IMAGE_PF_BGR32F );
        dn.Load( Cubemap2[3], nullptr, IMAGE_PF_BGR32F );
        bk.Load( Cubemap2[4], nullptr, IMAGE_PF_BGR32F );
        ft.Load( Cubemap2[5], nullptr, IMAGE_PF_BGR32F );
        w = cubeFaces[0]->GetWidth();
        cubemapCI.Resolution.TexCubemap.Width = w;
        cubemapCI.NumLods = 1;
        GDevice->CreateTexture( cubemapCI, &cubemap2 );
        for ( int face = 0 ; face < 6 ; face++ ) {
            float * pSrc = (float *)cubeFaces[face]->GetData();

            RenderCore::STextureRect rect = {};
            rect.Offset.Z = face;
            rect.Dimension.X = w;
            rect.Dimension.Y = w;
            rect.Dimension.Z = 1;

            cubemap2->WriteRect( rect, RenderCore::FORMAT_FLOAT3, w*w*3*sizeof( float ), 1, pSrc );
        }
    }

    RenderCore::ITexture * cubemaps[2] = { /*cubemap*/skybox, cubemap2 };
#else

    Texture cubemap;
    {
        AImage img;

        //img.Load( "052_hdrmaps_com_free.exr", NULL, IMAGE_PF_RGB16F ); 
        //img.Load( "059_hdrmaps_com_free.exr", NULL, IMAGE_PF_RGB16F );
        img.Load( "087_hdrmaps_com_free.exr", NULL, IMAGE_PF_RGB16F );

        RenderCore::ITexture source;
        RenderCore::TextureStorageCreateInfo createInfo = {};
        createInfo.Type = RenderCore::TEXTURE_2D;
        createInfo.Format = RenderCore::TEXTURE_FORMAT_RGB16F;
        createInfo.Resolution.Tex2D.Width = img.GetWidth();
        createInfo.Resolution.Tex2D.Height = img.GetHeight();
        createInfo.NumLods = 1;
        source = GDevice->CreateTexture( createInfo );
        source->Write( 0, RenderCore::PIXEL_FORMAT_HALF_RGB, img.GetWidth()*img.GetHeight()*3*2, 1, img.GetData() );

        const int cubemapResoultion = 1024;

        ACubemapGenerator cubemapGenerator;
        cubemapGenerator.Initialize();
        cubemapGenerator.Generate( cubemap, RenderCore::TEXTURE_FORMAT_RGB16F, cubemapResoultion, &source );

#if 0
        RenderCore::TextureRect rect;
        rect.Offset.Lod = 0;
        rect.Offset.X = 0;
        rect.Offset.Y = 0;
        rect.Dimension.X = cubemapResoultion;
        rect.Dimension.Y = cubemapResoultion;
        rect.Dimension.Z = 1;
        void * data = GHeapMemory.Alloc( cubemapResoultion*cubemapResoultion*3*sizeof( float ) );
        AFileStream f;
        for ( int i = 0 ; i < 6 ; i++ ) {
            rect.Offset.Z = i;
            cubemap.ReadRect( rect, RenderCore::FORMAT_FLOAT3, cubemapResoultion*cubemapResoultion*3*sizeof( float ), 1, data );
            f.OpenWrite( Core::Fmt( "nightsky_%d.hdr", i ) );
            WriteHDR( f, cubemapResoultion, cubemapResoultion, 3, (float*)data );
        }
        GHeapMemory.Free( data );
#endif
    }

    Texture * cubemaps[] = { &cubemap };
#endif

    

    {
        AEnvProbeGenerator envProbeGenerator;
        envProbeGenerator.GenerateArray( 7, AN_ARRAY_SIZE( cubemaps ), cubemaps, &PrefilteredMap );
        RenderCore::SSamplerInfo samplerCI;
        samplerCI.Filter = RenderCore::FILTER_MIPMAP_BILINEAR;
        samplerCI.bCubemapSeamless = true;

//!!!!!!!!!!!!
        GDevice->CreateBindlessSampler( PrefilteredMap, samplerCI, &PrefilteredMapBindless );

        //TRef< RenderCore::IBindlessSampler > smp;
        //GDevice->CreateBindlessSampler( PrefilteredMap, samplerCI, &smp );

        PrefilteredMapBindless->MakeResident();
    }

    {
        AIrradianceGenerator irradianceGenerator;
        irradianceGenerator.GenerateArray( AN_ARRAY_SIZE( cubemaps ), cubemaps, &IrradianceMap );
        RenderCore::SSamplerInfo samplerCI;
        samplerCI.Filter = RenderCore::FILTER_LINEAR;
        samplerCI.bCubemapSeamless = true;

        GDevice->CreateBindlessSampler( IrradianceMap, samplerCI, &IrradianceMapBindless );
        IrradianceMapBindless->MakeResident();
    }


}

void AFrameResources::Deinitialize() {
    ConstantBuffer.Reset();
    FrameConstantBuffer.Reset();
    WhiteTexture.Reset();
    SphereMesh.Reset();
    Saq.Reset();
    ClusterLookup.Reset();
    ClusterItemTBO.Reset();
    ClusterItemBuffer.Reset();
    PrefilteredMapBindless.Reset();
    IrradianceMapBindless.Reset();
    PrefilteredMap.Reset();
    IrradianceMap.Reset();
}

void AFrameResources::SetViewUniforms() {
    size_t offset = FrameConstantBuffer->Allocate( sizeof( SViewUniformBuffer ) );

    SViewUniformBuffer * uniformData = (SViewUniformBuffer *)(FrameConstantBuffer->GetMappedMemory() + offset);

    uniformData->OrthoProjection = GFrameData->OrthoProjection;
    uniformData->ViewProjection = GRenderView->ViewProjection;
    uniformData->ProjectionMatrix = GRenderView->ProjectionMatrix;
    uniformData->InverseProjectionMatrix = GRenderView->InverseProjectionMatrix;

    uniformData->InverseViewMatrix = GRenderView->ViewSpaceToWorldSpace;

    // Reprojection from viewspace to previous frame viewspace coordinates:
    // ViewspaceReprojection = WorldspaceToViewspacePrevFrame * ViewspaceToWorldspace
    uniformData->ViewspaceReprojection = GRenderView->ViewMatrixP * GRenderView->ViewSpaceToWorldSpace;

    // Reprojection from viewspace to previous frame projected coordinates:
    // ReprojectionMatrix = ProjectionMatrixPrevFrame * WorldspaceToViewspacePrevFrame * ViewspaceToWorldspace
    uniformData->ReprojectionMatrix = GRenderView->ProjectionMatrixP * uniformData->ViewspaceReprojection;

    uniformData->WorldNormalToViewSpace[0].X = GRenderView->NormalToViewMatrix[0][0];
    uniformData->WorldNormalToViewSpace[0].Y = GRenderView->NormalToViewMatrix[1][0];
    uniformData->WorldNormalToViewSpace[0].Z = GRenderView->NormalToViewMatrix[2][0];
    uniformData->WorldNormalToViewSpace[0].W = 0;

    uniformData->WorldNormalToViewSpace[1].X = GRenderView->NormalToViewMatrix[0][1];
    uniformData->WorldNormalToViewSpace[1].Y = GRenderView->NormalToViewMatrix[1][1];
    uniformData->WorldNormalToViewSpace[1].Z = GRenderView->NormalToViewMatrix[2][1];
    uniformData->WorldNormalToViewSpace[1].W = 0;

    uniformData->WorldNormalToViewSpace[2].X = GRenderView->NormalToViewMatrix[0][2];
    uniformData->WorldNormalToViewSpace[2].Y = GRenderView->NormalToViewMatrix[1][2];
    uniformData->WorldNormalToViewSpace[2].Z = GRenderView->NormalToViewMatrix[2][2];
    uniformData->WorldNormalToViewSpace[2].W = 0;

    uniformData->InvViewportSize.X = 1.0f / GRenderView->Width;
    uniformData->InvViewportSize.Y = 1.0f / GRenderView->Height;
    uniformData->ZNear = GRenderView->ViewZNear;
    uniformData->ZFar = GRenderView->ViewZFar;

    if ( GRenderView->bPerspective ) {
        uniformData->ProjectionInfo.X = -2.0f / GRenderView->ProjectionMatrix[0][0]; // (x) * (R - L)/N
        uniformData->ProjectionInfo.Y = 2.0f / GRenderView->ProjectionMatrix[1][1]; // (y) * (T - B)/N
        uniformData->ProjectionInfo.Z = (1.0f - GRenderView->ProjectionMatrix[2][0]) / GRenderView->ProjectionMatrix[0][0]; // L/N
        uniformData->ProjectionInfo.W = -(1.0f + GRenderView->ProjectionMatrix[2][1]) / GRenderView->ProjectionMatrix[1][1]; // B/N
    } else {
        uniformData->ProjectionInfo.X = 2.0f / GRenderView->ProjectionMatrix[0][0]; // (x) * R - L
        uniformData->ProjectionInfo.Y = -2.0f / GRenderView->ProjectionMatrix[1][1]; // (y) * T - B
        uniformData->ProjectionInfo.Z = -(1.0f + GRenderView->ProjectionMatrix[3][0]) / GRenderView->ProjectionMatrix[0][0]; // L
        uniformData->ProjectionInfo.W = (1.0f - GRenderView->ProjectionMatrix[3][1]) / GRenderView->ProjectionMatrix[1][1]; // B
    }

    uniformData->GameRunningTimeSeconds = GRenderView->GameRunningTimeSeconds;
    uniformData->GameplayTimeSeconds = GRenderView->GameplayTimeSeconds;

    uniformData->DynamicResolutionRatioX = (float)GRenderView->Width / GFrameData->AllocSurfaceWidth;
    uniformData->DynamicResolutionRatioY = (float)GRenderView->Height / GFrameData->AllocSurfaceHeight;

    uniformData->FeedbackBufferResolutionRatio = GRenderView->VTFeedback->GetResolutionRatio();
    uniformData->VTPageCacheCapacity.X = (float)GOpenGL45RenderBackend.VTWorkflow->PhysCache.GetPageCacheCapacityX();
    uniformData->VTPageCacheCapacity.Y = (float)GOpenGL45RenderBackend.VTWorkflow->PhysCache.GetPageCacheCapacityY();

    uniformData->VTPageTranslationOffsetAndScale = GOpenGL45RenderBackend.VTWorkflow->PhysCache.GetPageTranslationOffsetAndScale();

    uniformData->ViewPosition = GRenderView->ViewPosition;
    uniformData->TimeDelta = GRenderView->GameplayTimeStep;

    uniformData->PostprocessBloomMix = Float4( RVPostprocessBloomParam0.GetFloat(),
                                               RVPostprocessBloomParam1.GetFloat(),
                                               RVPostprocessBloomParam2.GetFloat(),
                                               RVPostprocessBloomParam3.GetFloat() ) * RVPostprocessBloomScale.GetFloat();

    uniformData->BloomEnabled = RVPostprocessBloom;  // TODO: Get from GRenderView
    uniformData->ToneMappingExposure = RVPostprocessToneExposure.GetFloat();  // TODO: Get from GRenderView
    uniformData->ColorGrading = GRenderView->CurrentColorGradingLUT ? 1.0f : 0.0f;
    uniformData->FXAA = RVFxaa;
    uniformData->VignetteColorIntensity = GRenderView->VignetteColorIntensity;
    uniformData->VignetteOuterRadiusSqr = GRenderView->VignetteOuterRadiusSqr;
    uniformData->VignetteInnerRadiusSqr = GRenderView->VignetteInnerRadiusSqr;
    uniformData->ColorGradingAdaptationSpeed = GRenderView->ColorGradingAdaptationSpeed;
    uniformData->ViewBrightness = Math::Saturate( RVBrightness.GetFloat() );

    uniformData->SSLRSampleOffset = RVSSLRSampleOffset.GetFloat();
    uniformData->SSLRMaxDist = RVSSLRMaxDist.GetFloat();
    uniformData->IsPerspective = float( GRenderView->bPerspective );
    uniformData->TessellationLevel = RVTessellationLevel.GetFloat() * GRenderView->Height;

    uniformData->PrefilteredMapSampler = PrefilteredMapBindless->GetHandle();
    uniformData->IrradianceMapSampler = IrradianceMapBindless->GetHandle();

    uniformData->DebugMode = RVDebugRenderMode.GetInteger();

    uniformData->NumDirectionalLights = GRenderView->NumDirectionalLights;
    //GLogger.Printf( "GRenderView->FirstDirectionalLight: %d\n", GRenderView->FirstDirectionalLight );

    for ( int i = 0 ; i < GRenderView->NumDirectionalLights ; i++ ) {
        SDirectionalLightDef * light = GFrameData->DirectionalLights[GRenderView->FirstDirectionalLight + i];

        uniformData->LightDirs[i] = Float4( GRenderView->NormalToViewMatrix * (light->Matrix[2]), 0.0f );
        uniformData->LightColors[i] = light->ColorAndAmbientIntensity;
        uniformData->LightParameters[i][0] = light->RenderMask;
        uniformData->LightParameters[i][1] = light->FirstCascade;
        uniformData->LightParameters[i][2] = light->NumCascades;
    }

    ViewUniformBufferBinding->BindingOffset = offset;
    ViewUniformBufferBinding->BindingSize = sizeof( *uniformData );

#if 0
    ViewUniformBufferBindingPostProcess->BindingOffset = offset;
    ViewUniformBufferBindingPostProcess->BindingSize = sizeof( *uniformData );
#endif
}

void AFrameResources::SetShadowMatrixBinding() {
    ShadowCascadeBinding->BindingOffset = ShadowMatrixBindingOffset;
    ShadowCascadeBinding->BindingSize = ShadowMatrixBindingSize;
}

void AFrameResources::SetShadowCascadeBinding( int FirstCascade, int NumCascades ) {
    ShadowCascadeBinding->BindingSize = MAX_SHADOW_CASCADES * sizeof( Float4x4 );
    ShadowCascadeBinding->BindingOffset = FrameConstantBuffer->Allocate( ShadowCascadeBinding->BindingSize );

    byte * pMemory = FrameConstantBuffer->GetMappedMemory() + ShadowCascadeBinding->BindingOffset;

    Core::Memcpy( pMemory, &GRenderView->LightViewProjectionMatrices[FirstCascade], NumCascades * sizeof( Float4x4 ) );

    ShadowCascadeBinding->pBuffer = FrameConstantBuffer->GetBuffer();
}

void AFrameResources::UploadUniforms() {
    SkeletonBufferBinding->pBuffer = GPUBufferHandle( GFrameData->StreamBuffer );
    SkeletonBufferBindingMB->pBuffer = GPUBufferHandle( GFrameData->StreamBuffer );

    SetViewUniforms();

    // Cascade matrices
    ShadowMatrixBindingSize = MAX_TOTAL_SHADOW_CASCADES_PER_VIEW * sizeof( Float4x4 );
    ShadowMatrixBindingOffset = FrameConstantBuffer->Allocate( ShadowMatrixBindingSize );

    byte * pMemory = FrameConstantBuffer->GetMappedMemory() + ShadowMatrixBindingOffset;

    Core::Memcpy( pMemory, GRenderView->ShadowMapMatrices, GRenderView->NumShadowMapCascades * sizeof( Float4x4 ) );

    // Light buffer
    LightBufferBinding->BindingSize = GRenderView->NumPointLights * sizeof( SClusterLight );
    LightBufferBinding->BindingOffset = FrameConstantBuffer->Allocate( LightBufferBinding->BindingSize );

    pMemory = FrameConstantBuffer->GetMappedMemory() + LightBufferBinding->BindingOffset;

    Core::Memcpy( pMemory, GRenderView->PointLights, LightBufferBinding->BindingSize );

    // IBL buffer
    IBLBufferBinding->BindingSize = GRenderView->NumProbes * sizeof( SClusterProbe );
    IBLBufferBinding->BindingOffset = FrameConstantBuffer->Allocate( IBLBufferBinding->BindingSize );

    pMemory = FrameConstantBuffer->GetMappedMemory() + IBLBufferBinding->BindingOffset;

    Core::Memcpy( pMemory, GRenderView->Probes, IBLBufferBinding->BindingSize );

    // Write cluster data
    ClusterLookup->Write( 0,
                          RenderCore::FORMAT_UINT2,
                          sizeof( SClusterData )*MAX_FRUSTUM_CLUSTERS_X*MAX_FRUSTUM_CLUSTERS_Y*MAX_FRUSTUM_CLUSTERS_Z,
                          1,
                          GRenderView->LightData.ClusterLookup );

    ClusterItemBuffer->WriteRange( 0,
                                   sizeof( SClusterItemBuffer )*GRenderView->LightData.TotalItems,
                                   GRenderView->LightData.ItemBuffer );
}
