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

#include "EnvironmentMap.h"
#include "Asset.h"
#include "AssetImporter.h"
#include "Engine.h"

HK_CLASS_META(AEnvironmentMap)

void AEnvironmentMap::InitializeFromImages(TArray<AImage, 6> const& Faces)
{
    int                 width;
    STexturePixelFormat pixelFormat;

    Purge();

    if (!ValidateCubemapFaces(Faces, width, pixelFormat))
    {
        InitializeDefaultObject();
        return;
    }

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolutionCubemap(width));
    textureDesc.SetFormat(pixelFormat.GetTextureFormat());
    textureDesc.SetMipLevels(1);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    if (pixelFormat.NumComponents() == 1)
    {
        // Apply texture swizzle for single channel textures
        textureDesc.Swizzle.R = RenderCore::TEXTURE_SWIZZLE_R;
        textureDesc.Swizzle.G = RenderCore::TEXTURE_SWIZZLE_R;
        textureDesc.Swizzle.B = RenderCore::TEXTURE_SWIZZLE_R;
        textureDesc.Swizzle.A = RenderCore::TEXTURE_SWIZZLE_R;
    }

    TRef<RenderCore::ITexture> cubemap;
    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &cubemap);

    size_t sizeInBytes = (size_t)width * width * pixelFormat.SizeInBytesUncompressed();

    RenderCore::STextureRect rect;
    rect.Offset.X        = 0;
    rect.Offset.Y        = 0;
    rect.Offset.MipLevel = 0;
    rect.Dimension.X     = width;
    rect.Dimension.Y     = width;
    rect.Dimension.Z     = 1;

    for (int faceNum = 0; faceNum < 6; faceNum++)
    {
        rect.Offset.Z = faceNum;

        cubemap->WriteRect(rect, pixelFormat.GetTextureDataFormat(), sizeInBytes, 1, Faces[faceNum].GetData());
    }

    GEngine->GetRenderBackend()->GenerateIrradianceMap(cubemap, &IrradianceMap);
    GEngine->GetRenderBackend()->GenerateReflectionMap(cubemap, &ReflectionMap);

    UpdateSamplers();
}

void AEnvironmentMap::Purge()
{
    IrradianceMap.Reset();
    ReflectionMap.Reset();
    IrradianceMapHandle = 0;
    ReflectionMapHandle = 0;
}

void AEnvironmentMap::CreateTextures(int IrradianceMapWidth, int ReflectionMapWidth)
{
    using namespace RenderCore;

    GEngine->GetRenderDevice()->CreateTexture(STextureDesc()
                                                  .SetFormat(TEXTURE_FORMAT_RGB16F)
                                                  .SetResolution(STextureResolutionCubemap(IrradianceMapWidth))
                                                  .SetBindFlags(BIND_SHADER_RESOURCE),
                                              &IrradianceMap);

    GEngine->GetRenderDevice()->CreateTexture(STextureDesc()
                                                  .SetFormat(TEXTURE_FORMAT_RGB16F)
                                                  .SetResolution(STextureResolutionCubemap(ReflectionMapWidth))
                                                  .SetMipLevels(Math::Log2((uint32_t)ReflectionMapWidth))
                                                  .SetBindFlags(BIND_SHADER_RESOURCE),
                                              &ReflectionMap);

    UpdateSamplers();
}

void AEnvironmentMap::UpdateSamplers()
{
    RenderCore::SSamplerDesc samplerCI;
    samplerCI.bCubemapSeamless = true;

    samplerCI.Filter    = RenderCore::FILTER_LINEAR;
    IrradianceMapHandle = IrradianceMap->GetBindlessSampler(samplerCI);
    IrradianceMap->MakeBindlessSamplerResident(IrradianceMapHandle, true);

    samplerCI.Filter    = RenderCore::FILTER_MIPMAP_BILINEAR;
    ReflectionMapHandle = ReflectionMap->GetBindlessSampler(samplerCI);
    ReflectionMap->MakeBindlessSamplerResident(ReflectionMapHandle, true);
}

bool AEnvironmentMap::LoadResource(IBinaryStreamReadInterface& Stream)
{
    Purge();

    uint32_t fileFormat = Stream.ReadUInt32();

    if (fileFormat != FMT_FILE_TYPE_ENVMAP)
    {
        LOG("Expected file format {}\n", FMT_FILE_TYPE_ENVMAP);
        return false;
    }

    uint32_t fileVersion = Stream.ReadUInt32();

    if (fileVersion != FMT_VERSION_ENVMAP)
    {
        LOG("Expected file version {}\n", FMT_VERSION_ENVMAP);
        return false;
    }

    int irradianceMapWidth = Stream.ReadUInt32();
    int reflectionMapWidth = Stream.ReadUInt32();
    int numReflectionMapMips = Math::Log2((uint32_t)reflectionMapWidth);

    CreateTextures(irradianceMapWidth, reflectionMapWidth);

    // Choose max width for memory allocation
    int maxSize = Math::Max(irradianceMapWidth, reflectionMapWidth);

    TPodVectorHeap<float> buffer(maxSize * maxSize * 3 * 6);

    void* data = buffer.ToPtr();

    int numFloats = irradianceMapWidth * irradianceMapWidth * 3 * 6;

    Stream.Read(data, numFloats * sizeof(float));

    for (int i = 0; i < numFloats; i++)
    {
        ((float*)data)[i] = Core::LittleFloat(((float*)data)[i]);
    }
    
    IrradianceMap->Write(0, RenderCore::FORMAT_FLOAT3, numFloats * sizeof(float), 4, data);

    for (int mipLevel = 0; mipLevel < numReflectionMapMips; mipLevel++)
    {
        int mipWidth = reflectionMapWidth >> mipLevel;
        HK_ASSERT(mipWidth > 0);

        numFloats = mipWidth * mipWidth * 3 * 6;

        Stream.Read(data, numFloats * sizeof(float));

        for (int i = 0; i < numFloats; i++)
        {
            ((float*)data)[i] = Core::LittleFloat(((float*)data)[i]);
        }

        ReflectionMap->Write(mipLevel, RenderCore::FORMAT_FLOAT3, numFloats * sizeof(float), 4, data);
    }

    return true;
}

void AEnvironmentMap::LoadInternalResource(const char* _Path)
{
    uint8_t color[4] = {10, 10, 10, 255};

    TArray<AImage, 6> faces;
    for (AImage& face : faces)
        face.FromRawData(color, 1, 1, nullptr, IMAGE_PF_BGRA_GAMMA2);
    
    InitializeFromImages(faces);
}
