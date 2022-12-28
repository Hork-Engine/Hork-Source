/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

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

#include "Level.h"
#include "Texture.h"
#include "Engine.h" // TODO: remove dependency

#include <Geometry/BV/BvIntersect.h>
#include <Geometry/ConvexHull.h>
#include <Core/IntrusiveLinkedListMacro.h>
#include <Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

void Level::OnAddLevelToWorld()
{
}

void Level::OnRemoveLevelFromWorld()
{
    DestroyActors();
}

void Level::DestroyActors()
{
    while (!m_Actors.IsEmpty())
        m_Actors.Last()->Destroy();
}

Float3 Level::SampleLight(int InLightmapBlock, Float2 const& InLighmapTexcoord) const
{
    if (!Lighting)
    {
        return Float3(1.0f);
    }

    return Lighting->SampleLight(InLightmapBlock, InLighmapTexcoord);
}

void Level::DrawDebug(DebugRenderer* InRenderer)
{

    //ConvexHull * hull = ConvexHull::CreateForPlane( PlaneF(Float3(0,0,1), Float3(0.0f) ), 100.0f );

    //InRenderer->SetDepthTest( false );
    //InRenderer->SetColor( Color4( 0, 1, 1, 0.5f ) );
    //InRenderer->DrawConvexPoly( hull->Points, hull->NumPoints, false );

    //Float3 normal = hull->CalcNormal();

    //InRenderer->SetColor( Color4::White() );
    //InRenderer->DrawLine( Float3(0.0f), Float3( 0, 0, 1 )*100.0f );

    //hull->Destroy();

#if 0
    TPodVector< BvAxisAlignedBox > clusters;
    clusters.Resize( PVSClustersCount );
    for ( BvAxisAlignedBox & box : clusters ) {
        box.Clear();
    }
    for ( BinarySpaceLeaf const & leaf : Leafs ) {
        if ( leaf.PVSCluster >= 0 && leaf.PVSCluster < PVSClustersCount ) {
            clusters[leaf.PVSCluster].AddAABB( leaf.Bounds );
        }
    }

    for ( BvAxisAlignedBox const & box : clusters ) {
        InRenderer->DrawAABB( box );
    }
    //LOG( "leafs {} clusters {}\n", Leafs.Size(), clusters.Size() );
#endif

    // Draw light portals
#if 0
    InRenderer->DrawTriangleSoup( LightPortalVertexBuffer, LightPortalIndexBuffer );
#endif
}

uint32_t Level::AddVertexLightChannel(IndexedMesh* InSourceMesh)
{
    uint32_t handle;
    if (!m_FreeVertexLightChannels.IsEmpty())
    {
        handle = m_FreeVertexLightChannels.Last();
        m_FreeVertexLightChannels.RemoveLast();
    }
    else
    {
        handle = m_VertexLightChannels.Size();
        m_VertexLightChannels.Add();
    }

    m_VertexLightChannels[handle] = new VertexLight(InSourceMesh);

    return handle;
}

void Level::RemoveVertexLightChannel(uint32_t VertexLightChannel)
{
    if (VertexLightChannel >= m_VertexLightChannels.Size() || m_VertexLightChannels[VertexLightChannel] == nullptr)
        return;

    m_VertexLightChannels[VertexLightChannel]->RemoveRef();
    m_VertexLightChannels[VertexLightChannel] = nullptr;

    m_FreeVertexLightChannels.Add(VertexLightChannel);
}

void Level::RemoveVertexLightChannels()
{
    for (VertexLight* vertexLight : m_VertexLightChannels)
    {
        vertexLight->RemoveRef();
    }
    m_VertexLightChannels.Free();
    m_FreeVertexLightChannels.Free();
}

VertexLight* Level::GetVertexLight(uint32_t VertexLightChannel)
{
    if (VertexLightChannel < m_VertexLightChannels.Size())
        return m_VertexLightChannels[VertexLightChannel];
    return nullptr;
}

LevelLighting::LevelLighting(LightingSystemCreateInfo const& CreateInfo)
{
    LightmapFormat      = CreateInfo.LightmapFormat;
    LightmapBlockWidth  = CreateInfo.LightmapBlockWidth;
    LightmapBlockHeight = CreateInfo.LightmapBlockHeight;
    if (CreateInfo.LightDataSize)
    {
        LightData = Platform::GetHeapAllocator<HEAP_MISC>().Alloc(CreateInfo.LightDataSize);
        Platform::Memcpy(LightData, CreateInfo.LightData, CreateInfo.LightDataSize);

        TEXTURE_FORMAT texFormat;
        RenderCore::TextureSwizzle swizzle;

        size_t blockSize = sizeof(uint16_t) * LightmapBlockWidth * LightmapBlockHeight;

        if (LightmapFormat == LIGHTMAP_RGBA16_FLOAT)
        {
            blockSize *= 4;
            texFormat = TEXTURE_FORMAT_RGBA16_FLOAT; // TODO: Check TEXTURE_FORMAT_R11G11B10_FLOAT
        }
        else
        {
            texFormat = TEXTURE_FORMAT_R16_FLOAT;
            swizzle.R  = RenderCore::TEXTURE_SWIZZLE_R;
            swizzle.G  = RenderCore::TEXTURE_SWIZZLE_R;
            swizzle.B  = RenderCore::TEXTURE_SWIZZLE_R;
            swizzle.A  = RenderCore::TEXTURE_SWIZZLE_R;
        }

        Lightmaps.Resize(CreateInfo.LightmapBlockCount);
        for (int blockNum = 0; blockNum < CreateInfo.LightmapBlockCount; blockNum++)
        {
            GEngine->GetRenderDevice()->CreateTexture(RenderCore::TextureDesc{}
                                                          .SetResolution(RenderCore::TextureResolution2D(LightmapBlockWidth, LightmapBlockHeight))
                                                          .SetFormat(texFormat)
                                                          .SetMipLevels(1)
                                                          .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE)
                                                          .SetSwizzle(swizzle),
                                                      &Lightmaps[blockNum]);
            Lightmaps[blockNum]->Write(0, blockSize, 2, (byte*)LightData + blockNum * blockSize);
            Lightmaps[blockNum]->SetDebugName("Lightmap block");
        }
    }

    RenderCore::BufferDesc bufferCI = {};
    bufferCI.MutableClientAccess     = RenderCore::MUTABLE_STORAGE_CLIENT_WRITE_ONLY;
    bufferCI.MutableUsage            = RenderCore::MUTABLE_STORAGE_STATIC;

    bufferCI.SizeInBytes = CreateInfo.ShadowCasterVertexCount * sizeof(Float3);
    GEngine->GetRenderDevice()->CreateBuffer(bufferCI, CreateInfo.ShadowCasterVertices, &ShadowCasterVB);
    ShadowCasterVB->SetDebugName("ShadowCasterVB");

    bufferCI.SizeInBytes = CreateInfo.ShadowCasterIndexCount * sizeof(unsigned int);
    GEngine->GetRenderDevice()->CreateBuffer(bufferCI, CreateInfo.ShadowCasterIndices, &ShadowCasterIB);
    ShadowCasterIB->SetDebugName("ShadowCasterIB");

    ShadowCasterIndexCount = CreateInfo.ShadowCasterIndexCount;

    LightPortals.Resize(CreateInfo.LightPortalsCount);
    Platform::Memcpy(LightPortals.ToPtr(), CreateInfo.LightPortals, CreateInfo.LightPortalsCount * sizeof(LightPortals[0]));

    LightPortalVertexBuffer.Resize(CreateInfo.LightPortalVertexCount);
    Platform::Memcpy(LightPortalVertexBuffer.ToPtr(), CreateInfo.LightPortalVertices, CreateInfo.LightPortalVertexCount * sizeof(LightPortalVertexBuffer[0]));

    LightPortalIndexBuffer.Resize(CreateInfo.LightPortalIndexCount);
    Platform::Memcpy(LightPortalIndexBuffer.ToPtr(), CreateInfo.LightPortalIndices, CreateInfo.LightPortalIndexCount * sizeof(LightPortalIndexBuffer[0]));

    bufferCI.SizeInBytes = LightPortalVertexBuffer.Size() * sizeof(Float3);
    GEngine->GetRenderDevice()->CreateBuffer(bufferCI, LightPortalVertexBuffer.ToPtr(), &LightPortalsVB);
    LightPortalsVB->SetDebugName("LightPortalVertexBuffer");

    bufferCI.SizeInBytes = LightPortalIndexBuffer.Size() * sizeof(unsigned int);
    GEngine->GetRenderDevice()->CreateBuffer(bufferCI, LightPortalIndexBuffer.ToPtr(), &LightPortalsIB);
    LightPortalsIB->SetDebugName("LightPortalIndexBuffer");
}

LevelLighting::~LevelLighting()
{
    Platform::GetHeapAllocator<HEAP_MISC>().Free(LightData);
}

Float3 LevelLighting::SampleLight(int InLightmapBlock, Float2 const& InLighmapTexcoord) const
{
    if (!LightData)
    {
        return Float3(1.0f);
    }

    HK_ASSERT(InLightmapBlock >= 0 && InLightmapBlock < Lightmaps.Size());

    int numChannels = (LightmapFormat == LIGHTMAP_GRAYSCALED16_FLOAT) ? 1 : 4;
    int blockSize   = LightmapBlockWidth * LightmapBlockHeight * numChannels;

    const Half* src = (const Half*)LightData + InLightmapBlock * blockSize;

    const float  sx = Math::Clamp(InLighmapTexcoord.X, 0.0f, 1.0f) * (LightmapBlockWidth - 1);
    const float  sy = Math::Clamp(InLighmapTexcoord.Y, 0.0f, 1.0f) * (LightmapBlockHeight - 1);
    const Float2 lerp(Math::Fract(sx), Math::Fract(sy));

    const int x0 = Math::ToIntFast(sx);
    const int y0 = Math::ToIntFast(sy);
    int       x1 = x0 + 1;
    if (x1 >= LightmapBlockWidth)
        x1 = LightmapBlockWidth - 1;
    int y1 = y0 + 1;
    if (y1 >= LightmapBlockHeight)
        y1 = LightmapBlockHeight - 1;

    const int offset00 = (y0 * LightmapBlockWidth + x0) * numChannels;
    const int offset10 = (y0 * LightmapBlockWidth + x1) * numChannels;
    const int offset01 = (y1 * LightmapBlockWidth + x0) * numChannels;
    const int offset11 = (y1 * LightmapBlockWidth + x1) * numChannels;

    const Half* src00 = src + offset00;
    const Half* src10 = src + offset10;
    const Half* src01 = src + offset01;
    const Half* src11 = src + offset11;

    Float3 light(0);

    switch (LightmapFormat)
    {
        case LIGHTMAP_GRAYSCALED16_FLOAT: {
            light[0] = light[1] = light[2] = Math::Bilerp(float(src00[0]), float(src10[0]), float(src01[0]), float(src11[0]), lerp);
            break;
        }
        case LIGHTMAP_RGBA16_FLOAT: {
            for (int i = 0; i < 3; i++)
            {
                light[i] = Math::Bilerp(float(src00[i]), float(src10[i]), float(src01[i]), float(src11[i]), lerp);
            }
            break;
        }
        default:
            LOG("Level::SampleLight: Unknown lightmap format\n");
            break;
    }

    return light;
}

LevelAudio::LevelAudio(LevelAudioCreateInfo const& CreateInfo)
{
    AudioAreas.Resize(CreateInfo.NumAudioAreas);
    Platform::Memcpy(AudioAreas.ToPtr(), CreateInfo.AudioAreas, CreateInfo.NumAudioAreas * sizeof(AudioAreas[0]));

    AmbientSounds = CreateInfo.AmbientSounds;
}

HK_NAMESPACE_END
