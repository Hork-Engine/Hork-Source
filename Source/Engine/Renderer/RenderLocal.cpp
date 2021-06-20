/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2021 Alexander Samusev.

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

#include "RenderLocal.h"

#include <Core/Public/Core.h>
#include <Runtime/Public/Runtime.h>

// NOTE: supported by NVidia, but not supported on AMD
//#define CSTYLE_LINE_DIRECTIVE

ARuntimeVariable r_MaterialDebugMode( _CTS( "r_MaterialDebugMode" ),
                                      #ifdef AN_DEBUG
                                      _CTS( "1" ),
                                      #else
                                      _CTS( "0" ),
                                      #endif
                                      VAR_CHEAT );

/** Render device */
RenderCore::IDevice * GDevice;

/** Render context */
RenderCore::IImmediateContext * rcmd;

/** Render resource table */
RenderCore::IResourceTable * rtbl;

/** Render frame data */
SRenderFrame * GFrameData;

/** Render frame view */
SRenderView * GRenderView;

/** Render view area */
ARenderArea GRenderViewArea;

/** Stream buffer */
RenderCore::IBuffer * GStreamBuffer;

/** Circular buffer. Contains constant data for single draw call.
Don't use to store long-live data. */
TRef< ACircularBuffer > GCircularBuffer;

/** Sphere mesh */
TRef< ASphereMesh > GSphereMesh;

/** Screen aligned quad mesh */
TRef< RenderCore::IBuffer > GSaq;

/** Simple white texture */
TRef< RenderCore::ITexture > GWhiteTexture;

/** Cluster lookcup 3D texture */
TRef< RenderCore::ITexture > GClusterLookup;

/** Cluster item references */
TRef< RenderCore::IBuffer > GClusterItemBuffer;

/** Cluster item references view */
TRef< RenderCore::IBufferView > GClusterItemTBO;

/** Irradiance texture array */
TRef< RenderCore::ITexture > GIrradianceMap;
TRef< RenderCore::IBindlessSampler > GIrradianceMapBindless;

/** Reflections texture array */
TRef< RenderCore::ITexture > GPrefilteredMap;
TRef< RenderCore::IBindlessSampler > GPrefilteredMapBindless;

/** View constant binding */
size_t GViewConstantBufferBindingBindingOffset;
size_t GViewConstantBufferBindingBindingSize;

AVirtualTextureFeedbackAnalyzer * GFeedbackAnalyzerVT;
AVirtualTextureCache * GPhysCacheVT;

RenderCore::IPipeline * GTerrainDepthPipeline;
RenderCore::IPipeline * GTerrainLightPipeline;
RenderCore::IPipeline * GTerrainWireframePipeline;

RenderCore::STextureResolution2D GetFrameResoultion()
{
    return RenderCore::STextureResolution2D( GFrameData->RenderTargetMaxWidth, GFrameData->RenderTargetMaxHeight );
}

void DrawSAQ( RenderCore::IPipeline * Pipeline, unsigned int InstanceCount )
{
    const RenderCore::SDrawCmd drawCmd = { 4, InstanceCount, 0, 0 };
    rcmd->BindPipeline( Pipeline );
    rcmd->BindVertexBuffer( 0, GSaq, 0 );
    rcmd->BindIndexBuffer( NULL, RenderCore::INDEX_TYPE_UINT16, 0 );
    rcmd->Draw( &drawCmd );
}

void DrawSphere( RenderCore::IPipeline * Pipeline, unsigned int InstanceCount )
{
    RenderCore::SDrawIndexedCmd drawCmd = {};
    drawCmd.IndexCountPerInstance = GSphereMesh->IndexCount;
    drawCmd.InstanceCount = InstanceCount;

    rcmd->BindPipeline( Pipeline );
    rcmd->BindVertexBuffer( 0, GSphereMesh->VertexBuffer );
    rcmd->BindIndexBuffer( GSphereMesh->IndexBuffer, RenderCore::INDEX_TYPE_UINT16 );
    rcmd->Draw( &drawCmd );
}

void BindTextures( RenderCore::IResourceTable * Rtbl, SMaterialFrameData * Instance, int MaxTextures )
{
    AN_ASSERT( Instance );

    int n = Math::Min( Instance->NumTextures, MaxTextures );

    for ( int t = 0 ; t < n ; t++ ) {
        Rtbl->BindTexture( t, Instance->Textures[t] );
    }
}

void BindTextures( SMaterialFrameData * Instance, int MaxTextures )
{
    AN_ASSERT( Instance );

    int n = Math::Min( Instance->NumTextures, MaxTextures );

    for ( int t = 0 ; t < n ; t++ ) {
        rtbl->BindTexture( t, Instance->Textures[t] );
    }
}

void BindVertexAndIndexBuffers( SRenderInstance const * Instance )
{
    rcmd->BindVertexBuffer( 0, Instance->VertexBuffer, Instance->VertexBufferOffset );
    rcmd->BindIndexBuffer( Instance->IndexBuffer, RenderCore::INDEX_TYPE_UINT32, Instance->IndexBufferOffset );
}

void BindVertexAndIndexBuffers( SShadowRenderInstance const * Instance )
{
    rcmd->BindVertexBuffer( 0, Instance->VertexBuffer, Instance->VertexBufferOffset );
    rcmd->BindIndexBuffer( Instance->IndexBuffer, RenderCore::INDEX_TYPE_UINT32, Instance->IndexBufferOffset );
}

void BindVertexAndIndexBuffers( SLightPortalRenderInstance const * Instance )
{
    rcmd->BindVertexBuffer( 0, Instance->VertexBuffer, Instance->VertexBufferOffset );
    rcmd->BindIndexBuffer( Instance->IndexBuffer, RenderCore::INDEX_TYPE_UINT32, Instance->IndexBufferOffset );
}

void BindSkeleton( size_t _Offset, size_t _Size )
{
    rtbl->BindBuffer( 2, GStreamBuffer, _Offset, _Size );
}

void BindSkeletonMotionBlur( size_t _Offset, size_t _Size )
{
    rtbl->BindBuffer( 7, GStreamBuffer, _Offset, _Size );
}

void BindInstanceConstants( SRenderInstance const * Instance )
{
    size_t offset = GCircularBuffer->Allocate( sizeof( SInstanceConstantBuffer ) );

    SInstanceConstantBuffer * pConstantBuf = reinterpret_cast< SInstanceConstantBuffer * >(GCircularBuffer->GetMappedMemory() + offset);

    Core::Memcpy( &pConstantBuf->TransformMatrix, &Instance->Matrix, sizeof( pConstantBuf->TransformMatrix ) );
    Core::Memcpy( &pConstantBuf->TransformMatrixP, &Instance->MatrixP, sizeof( pConstantBuf->TransformMatrixP ) );
    StoreFloat3x3AsFloat3x4Transposed( Instance->ModelNormalToViewSpace, pConstantBuf->ModelNormalToViewSpace );
    Core::Memcpy( &pConstantBuf->LightmapOffset, &Instance->LightmapOffset, sizeof( pConstantBuf->LightmapOffset ) );
    Core::Memcpy( &pConstantBuf->uaddr_0, Instance->MaterialInstance->UniformVectors, sizeof( Float4 )*Instance->MaterialInstance->NumUniformVectors );

    // TODO:
    pConstantBuf->VTOffset = Float2( 0.0f );//Instance->VTOffset;
    pConstantBuf->VTScale = Float2( 1.0f );//Instance->VTScale;
    pConstantBuf->VTUnit = 0;//Instance->VTUnit;

    rtbl->BindBuffer( 1, GCircularBuffer->GetBuffer(), offset, sizeof( SInstanceConstantBuffer ) );
}

void BindInstanceConstantsFB( SRenderInstance const * Instance )
{
    size_t offset = GCircularBuffer->Allocate( sizeof( SFeedbackConstantBuffer ) );

    SFeedbackConstantBuffer * pConstantBuf = reinterpret_cast< SFeedbackConstantBuffer * >(GCircularBuffer->GetMappedMemory() + offset);

    Core::Memcpy( &pConstantBuf->TransformMatrix, &Instance->Matrix, sizeof( pConstantBuf->TransformMatrix ) );

    // TODO:
    pConstantBuf->VTOffset = Float2(0.0f);//Instance->VTOffset;
    pConstantBuf->VTScale = Float2(1.0f);//Instance->VTScale;
    pConstantBuf->VTUnit = 0;//Instance->VTUnit;

    rtbl->BindBuffer( 1, GCircularBuffer->GetBuffer(), offset, sizeof( SFeedbackConstantBuffer ) );
}

void BindShadowInstanceConstants( SShadowRenderInstance const * Instance )
{
    size_t offset = GCircularBuffer->Allocate( sizeof( SShadowInstanceConstantBuffer ) );

    SShadowInstanceConstantBuffer * pConstantBuf = reinterpret_cast< SShadowInstanceConstantBuffer * >(GCircularBuffer->GetMappedMemory() + offset);

    StoreFloat3x4AsFloat4x4Transposed( Instance->WorldTransformMatrix, pConstantBuf->TransformMatrix );

    if ( Instance->MaterialInstance ) {
        Core::Memcpy( &pConstantBuf->uaddr_0, Instance->MaterialInstance->UniformVectors, sizeof( Float4 )*Instance->MaterialInstance->NumUniformVectors );
    }

    pConstantBuf->CascadeMask = Instance->CascadeMask;

    rtbl->BindBuffer( 1, GCircularBuffer->GetBuffer(), offset, sizeof( SShadowInstanceConstantBuffer ) );
}

void * MapDrawCallConstants( size_t SizeInBytes )
{
    size_t offset = GCircularBuffer->Allocate( SizeInBytes );

    rtbl->BindBuffer( 1, GCircularBuffer->GetBuffer(), offset, SizeInBytes );

    return GCircularBuffer->GetMappedMemory() + offset;
}

void BindShadowMatrix()
{
    rtbl->BindBuffer( 3, GStreamBuffer, GRenderView->ShadowMapMatricesStreamHandle, MAX_TOTAL_SHADOW_CASCADES_PER_VIEW * sizeof( Float4x4 ) );
}

void BindShadowCascades( size_t StreamHandle )
{
    rtbl->BindBuffer( 3, GStreamBuffer, StreamHandle, MAX_SHADOW_CASCADES * sizeof( Float4x4 ) );
}

void CreateFullscreenQuadPipeline( TRef< RenderCore::IPipeline > * ppPipeline, AStringView VertexShader, AStringView FragmentShader, RenderCore::SPipelineResourceLayout const * pResourceLayout, RenderCore::BLENDING_PRESET BlendingPreset )
{
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

    CreateVertexShader( VertexShader, vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ), pipelineCI.pVS );
    CreateFragmentShader( FragmentShader, pipelineCI.pFS );

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;
    inputAssembly.bPrimitiveRestart = false;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float2 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    if ( pResourceLayout ) {
        pipelineCI.ResourceLayout = *pResourceLayout;
    }

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void CreateFullscreenQuadPipelineGS( TRef< RenderCore::IPipeline > * ppPipeline, AStringView VertexShader, AStringView FragmentShader, AStringView GeometryShader, RenderCore::SPipelineResourceLayout const * pResourceLayout, RenderCore::BLENDING_PRESET BlendingPreset )
{
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

    CreateVertexShader( VertexShader, vertexAttribs, AN_ARRAY_SIZE( vertexAttribs ), pipelineCI.pVS );
    CreateGeometryShader( GeometryShader, pipelineCI.pGS );
    CreateFragmentShader( FragmentShader, pipelineCI.pFS );

    SPipelineInputAssemblyInfo & inputAssembly = pipelineCI.IA;
    inputAssembly.Topology = PRIMITIVE_TRIANGLE_STRIP;
    inputAssembly.bPrimitiveRestart = false;

    SVertexBindingInfo vertexBinding[1] = {};

    vertexBinding[0].InputSlot = 0;
    vertexBinding[0].Stride = sizeof( Float2 );
    vertexBinding[0].InputRate = INPUT_RATE_PER_VERTEX;

    pipelineCI.NumVertexBindings = AN_ARRAY_SIZE( vertexBinding );
    pipelineCI.pVertexBindings = vertexBinding;

    pipelineCI.NumVertexAttribs = AN_ARRAY_SIZE( vertexAttribs );
    pipelineCI.pVertexAttribs = vertexAttribs;

    if ( pResourceLayout ) {
        pipelineCI.ResourceLayout = *pResourceLayout;
    }

    GDevice->CreatePipeline( pipelineCI, ppPipeline );
}

void SaveSnapshot( RenderCore::ITexture & _Texture )
{
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

struct SIncludeCtx
{
    /** Callback for file loading */
    bool ( *LoadFile )( AStringView FileName, AString & Source );

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

static void AddInclude( SIncludeInfo *& list, SIncludeInfo *& prev, int offset, int end, const char *filename, int len, int next_line )
{
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
    }
    else {
        list = prev = z;
    }
}

static void FreeIncludes( SIncludeInfo * list )
{
    SIncludeInfo * next;
    for ( SIncludeInfo * includeInfo = list ; includeInfo ; includeInfo = next ) {
        next = includeInfo->next;
        GZoneMemory.Free( includeInfo );
    }
}

static AN_FORCEINLINE int IsSpace( int ch )
{
    return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

// find location of all #include
static SIncludeInfo * FindIncludes( AStringView text )
{
    int line_count = 1;
    const char *s = text.Begin(), *start;
    const char *e = text.End();
    SIncludeInfo *list = NULL, *prev = NULL;
    while ( s < e ) {
        // parse is always at start of line when we reach here
        start = s;
        while ( ( *s == ' ' || *s == '\t' ) && s < e ) {
            ++s;
        }
        if ( s < e && *s == '#' ) {
            ++s;
            while ( ( *s == ' ' || *s == '\t' ) && s < e )
                ++s;
            if ( e-s > 7 && !Core::StrcmpN( s, "include", 7 ) && IsSpace( s[7] ) ) {
                s += 7;
                while ( ( *s == ' ' || *s == '\t' ) && s < e )
                    ++s;
                if ( *s == '"' && s < e ) {
                    const char *t = ++s;
                    while ( *t != '"' && *t != '\n' && *t != '\r' && t < e )
                        ++t;
                    if ( *t == '"' && t < e ) {
                        int len = t - s;
                        const char * filename = s;
                        s = t;
                        while ( *s != '\r' && *s != '\n' && s < e )
                            ++s;
                        // s points to the newline, so s-start is everything except the newline
                        AddInclude( list, prev, start-text.Begin(), s-text.Begin(), filename, len, line_count+1 );
                    }
                }
            }
        }
        while ( *s != '\r' && *s != '\n' && s < e )
            ++s;
        if ( ( *s == '\r' || *s == '\n' ) && s < e ) {
            s = s + (s[0] + s[1] == '\r' + '\n' ? 2 : 1);
        }
        ++line_count;
    }
    return list;
}

static void CleanComments( char * s )
{
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
                    }
                    else {
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

static bool LoadShaderWithInclude( SIncludeCtx * Ctx, AStringView FileName, AString & Out );

static bool LoadShaderFromString( SIncludeCtx * Ctx, AStringView FileName, AStringView Source, AString & Out )
{
    char temp[4096];
    SIncludeInfo * includeList = FindIncludes( Source );
    size_t sourceOffset = 0;

    for ( SIncludeInfo * includeInfo = includeList ; includeInfo ; includeInfo = includeInfo->next ) {
        Out += AStringView( &Source[sourceOffset], includeInfo->offset - sourceOffset );

        if ( Ctx->Predefined && includeInfo->filename[0] == '$' ) {
            // predefined source
#ifdef CSTYLE_LINE_DIRECTIVE
            Out += "#line 1 \"";
            Out += AStringView( includeInfo->filename, includeInfo->len );
            Out += "\"\n";
#else
            Out += "#line 1\n";
#endif
            SMaterialShader const * s;
            for ( s = Ctx->Predefined ; s ; s = s->Next ) {
                if ( !Core::StricmpN( s->SourceName, includeInfo->filename, includeInfo->len ) ) {
                    break;
                }
            }

            if ( !s || !LoadShaderFromString( Ctx, FileName, AStringView(s->Code), Out ) ) {
                FreeIncludes( includeList );
                return false;
            }
        }
        else {
#ifdef CSTYLE_LINE_DIRECTIVE
            Out += "#line 1 \"";
            Out += AStringView( includeInfo->filename, includeInfo->len );
            Out += "\"\n";
#else
            Out += "#line 1\n";
#endif
            temp[0] = 0;
            Core::StrcatN( temp, sizeof( temp ), includeInfo->filename, includeInfo->len );
            if ( !LoadShaderWithInclude( Ctx, temp, Out ) ) {
                FreeIncludes( includeList );
                return false;
            }
        }        

#ifdef CSTYLE_LINE_DIRECTIVE
        Core::Sprintf( temp, sizeof( temp ), "\n#line %d \"%s\"", includeInfo->next_line_after, FileName ? FileName : "source-file" );
#else
        Core::Sprintf( temp, sizeof( temp ), "\n#line %d", includeInfo->next_line_after );
#endif
        Out += temp;

        sourceOffset = includeInfo->end;
    }

    Out += AStringView( &Source[sourceOffset], Source.Length() - sourceOffset );
    FreeIncludes( includeList );

    return true;
}

static bool LoadShaderWithInclude( SIncludeCtx * Ctx, AStringView FileName, AString & Out )
{
    AString source;

    if ( !Ctx->LoadFile( FileName, source ) ) {
        GLogger.Printf( "Couldn't load %s\n", FileName.ToString().CStr() );
        return false;
    }

    CleanComments( source.ToPtr() );

    return LoadShaderFromString( Ctx, FileName, source, Out );
}

ARuntimeVariable r_EmbeddedShaders( _CTS("r_EmbeddedShaders"), _CTS("1") );

static bool GetShaderSource( AStringView FileName, AString & Source )
{
    if ( r_EmbeddedShaders ) {
        AMemoryStream f;
        if ( !f.OpenRead( "Shaders/" + FileName, GRuntime->GetEmbeddedResources() ) ) {
            return false;
        }
        Source.FromFile( f );
    }
    else {
        // Load shaders from sources
        AString fn = __FILE__;
        fn.ClipFilename();
        fn += "/../Embedded/Shaders/";
        fn += FileName;
        fn.FixPath();
        AFileStream f;
        if ( !f.OpenRead( fn.CStr() ) ) {
            return false;
        }
        Source.FromFile( f );
    }

    return true;
}

AString LoadShader( AStringView FileName, SMaterialShader const * Predefined )
{
    SIncludeCtx ctx;
    ctx.LoadFile = GetShaderSource;
    ctx.Predefined = Predefined;

    AString result;
#ifdef CSTYLE_LINE_DIRECTIVE
    result += "#line 1 \"";
    result += FileName;
    result += "\"\n";
#else
    result += "#line 1\n";
#endif

    if ( !LoadShaderWithInclude( &ctx, FileName, result ) ) {
        CriticalError( "LoadShader: failed to open %s\n", FileName.ToString().CStr() );
    }

    return result;
}

AString LoadShaderFromString( AStringView FileName, AStringView Source, SMaterialShader const * Predefined )
{
    SIncludeCtx ctx;
    ctx.LoadFile = GetShaderSource;
    ctx.Predefined = Predefined;

    AString result;
#ifdef CSTYLE_LINE_DIRECTIVE
    result += "#line 1 \"";
    result += FileName;
    result += "\"\n";
#else
    result += "#line 1\n";
#endif

    AString source = Source;

    CleanComments( source.ToPtr() );

    if ( !LoadShaderFromString( &ctx, FileName, source, result ) ) {
        CriticalError( "LoadShader: failed to open %s\n", FileName.ToString().CStr() );
    }

    return result;
}

void CreateShader( RenderCore::SHADER_TYPE _ShaderType, TPodVector< const char * > _SourcePtrs, TRef< RenderCore::IShaderModule > & _Module )
{
    TPodVector< const char * > sources;

    const char * predefine[] = {
        "#define VERTEX_SHADER\n",
        "#define FRAGMENT_SHADER\n",
        "#define TESS_CONTROL_SHADER\n",
        "#define TESS_EVALUATION_SHADER\n",
        "#define GEOMETRY_SHADER\n",
        "#define COMPUTE_SHADER\n"
    };

    AString predefines = predefine[_ShaderType];

    switch ( GDevice->GetGraphicsVendor() ) {
    case RenderCore::VENDOR_NVIDIA:
        predefines += "#define NVIDIA\n";
        break;
    case RenderCore::VENDOR_ATI:
        predefines += "#define ATI\n";
        break;
    case RenderCore::VENDOR_INTEL:
        predefines += "#define INTEL\n";
        break;
    default:
        // skip "not handled enumeration" warning
        break;
    }

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
    if ( r_MaterialDebugMode ) {
        predefines += "#define DEBUG_RENDER_MODE\n";
    }

    predefines += "#define SRGB_GAMMA_APPROX\n";

    if ( r_SSLR ) {
        predefines += "#define WITH_SSLR\n";
    }

    if ( r_HBAO ) {
        predefines += "#define WITH_SSAO\n";
    }

    sources.Append( "#version 450\n" );
    sources.Append( "#extension GL_ARB_bindless_texture : enable\n" );
    sources.Append( predefines.CStr() );
    sources.Append( _SourcePtrs );

    // Print sources
#if 0
    for ( int i = 0 ; i < sources.Size() ; i++ ) {
        GLogger.Printf( "%s\n", i, sources[i] );
    }
#endif

    using namespace RenderCore;

    GDevice->CreateShaderFromCode( _ShaderType, sources.Size(), sources.ToPtr(), &_Module );
}

void CreateShader( RenderCore::SHADER_TYPE _ShaderType, const char * _SourcePtr, TRef< RenderCore::IShaderModule > & _Module )
{
    TPodVector< const char * > sources;
    sources.Append( _SourcePtr );
    CreateShader( _ShaderType, sources, _Module );
}

void CreateShader( RenderCore::SHADER_TYPE _ShaderType, AString const & _SourcePtr, TRef< RenderCore::IShaderModule > & _Module )
{
    CreateShader( _ShaderType, _SourcePtr.CStr(), _Module );
}

void CreateVertexShader( AStringView FileName, RenderCore::SVertexAttribInfo const * _VertexAttribs, int _NumVertexAttribs, TRef< RenderCore::IShaderModule > & _Module )
{
    // TODO: here check if the shader binary is cached. Load from cache if so.

    AString vertexAttribsShaderString = RenderCore::ShaderStringForVertexAttribs< AString >( _VertexAttribs, _NumVertexAttribs );
    AString source = LoadShader( FileName );

    TPodVector< const char * > sources;

    sources.Append( vertexAttribsShaderString.CStr() );
    sources.Append( source.CStr() );

    CreateShader( RenderCore::VERTEX_SHADER, sources, _Module );

    // TODO: Write shader binary to cache
}

void CreateTessControlShader( AStringView FileName, TRef< RenderCore::IShaderModule > & _Module )
{
    AString source = LoadShader( FileName );
    CreateShader( RenderCore::TESS_CONTROL_SHADER, source, _Module );
}

void CreateTessEvalShader( AStringView FileName, TRef< RenderCore::IShaderModule > & _Module )
{
    AString source = LoadShader( FileName );
    CreateShader( RenderCore::TESS_EVALUATION_SHADER, source, _Module );
}

void CreateGeometryShader( AStringView FileName, TRef< RenderCore::IShaderModule > & _Module )
{
    AString source = LoadShader( FileName );
    CreateShader( RenderCore::GEOMETRY_SHADER, source, _Module );
}

void CreateFragmentShader( AStringView FileName, TRef< RenderCore::IShaderModule > & _Module )
{
    AString source = LoadShader( FileName );
    CreateShader( RenderCore::FRAGMENT_SHADER, source, _Module );
}
