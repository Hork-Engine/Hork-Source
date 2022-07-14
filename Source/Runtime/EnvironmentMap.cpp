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

void AEnvironmentMap::InitializeFromImage(ImageStorage const& Image)
{
    int width = Image.GetDesc().Width;

    Purge();

    if (Image.GetDesc().SliceCount < 6)
    {
        InitializeDefaultObject();
        return;
    }

    RenderCore::STextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::STextureResolutionCubemap(width));
    textureDesc.SetFormat(Image.GetDesc().Format);
    textureDesc.SetMipLevels(1);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    if (Image.NumChannels() == 1)
    {
        // Apply texture swizzle for single channel textures
        textureDesc.Swizzle.R = RenderCore::TEXTURE_SWIZZLE_R;
        textureDesc.Swizzle.G = RenderCore::TEXTURE_SWIZZLE_R;
        textureDesc.Swizzle.B = RenderCore::TEXTURE_SWIZZLE_R;
        textureDesc.Swizzle.A = RenderCore::TEXTURE_SWIZZLE_R;
    }

    TRef<RenderCore::ITexture> cubemap;
    GEngine->GetRenderDevice()->CreateTexture(textureDesc, &cubemap);

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

        ImageViewDesc    view;
        view.FirstSlice  = faceNum;
        view.SliceCount  = 1;
        view.MipmapIndex = 0;

        ImageSubresource subresource = Image.GetSubresource(view);

        cubemap->WriteRect(rect, subresource.GetSizeInBytes(), 1, subresource.GetData());
    }

    GEngine->GetRenderBackend()->GenerateIrradianceMap(cubemap, &m_IrradianceMap);
    GEngine->GetRenderBackend()->GenerateReflectionMap(cubemap, &m_ReflectionMap);

    m_IrradianceMap->SetDebugName("Irradiance Map");
    m_ReflectionMap->SetDebugName("Reflection Map");

    UpdateSamplers();
}

void AEnvironmentMap::Purge()
{
    m_IrradianceMap.Reset();
    m_ReflectionMap.Reset();
    m_IrradianceMapHandle = 0;
    m_ReflectionMapHandle = 0;
}

void AEnvironmentMap::CreateTextures(int IrradianceMapWidth, int ReflectionMapWidth)
{
    using namespace RenderCore;

    GEngine->GetRenderDevice()->CreateTexture(STextureDesc()
                                                  .SetFormat(TEXTURE_FORMAT_R11G11B10_FLOAT)
                                                  .SetResolution(STextureResolutionCubemap(IrradianceMapWidth))
                                                  .SetBindFlags(BIND_SHADER_RESOURCE),
                                              &m_IrradianceMap);

    GEngine->GetRenderDevice()->CreateTexture(STextureDesc()
                                                  .SetFormat(TEXTURE_FORMAT_R11G11B10_FLOAT)
                                                  .SetResolution(STextureResolutionCubemap(ReflectionMapWidth))
                                                  .SetMipLevels(Math::Log2((uint32_t)ReflectionMapWidth))
                                                  .SetBindFlags(BIND_SHADER_RESOURCE),
                                              &m_ReflectionMap);

    m_IrradianceMap->SetDebugName("Irradiance Map");
    m_ReflectionMap->SetDebugName("Reflection Map");

    UpdateSamplers();
}

void AEnvironmentMap::UpdateSamplers()
{
    RenderCore::SSamplerDesc samplerCI;
    samplerCI.bCubemapSeamless = true;

    samplerCI.Filter    = RenderCore::FILTER_LINEAR;
    m_IrradianceMapHandle = m_IrradianceMap->GetBindlessSampler(samplerCI);
    m_IrradianceMap->MakeBindlessSamplerResident(m_IrradianceMapHandle, true);

    samplerCI.Filter    = RenderCore::FILTER_MIPMAP_BILINEAR;
    m_ReflectionMapHandle = m_ReflectionMap->GetBindlessSampler(samplerCI);
    m_ReflectionMap->MakeBindlessSamplerResident(m_ReflectionMapHandle, true);
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

    TVector<uint32_t> buffer(maxSize * maxSize * 6);

    uint32_t* data = buffer.ToPtr();

    int numPixels = irradianceMapWidth * irradianceMapWidth * 6;

    Stream.ReadWords<uint32_t>(data, numPixels);

    m_IrradianceMap->Write(0, numPixels * sizeof(uint32_t), 4, data);

    for (int mipLevel = 0; mipLevel < numReflectionMapMips; mipLevel++)
    {
        int mipWidth = reflectionMapWidth >> mipLevel;
        HK_ASSERT(mipWidth > 0);

        numPixels = mipWidth * mipWidth * 6;

        Stream.ReadWords<uint32_t>(data, numPixels);

        m_ReflectionMap->Write(mipLevel, numPixels * sizeof(uint32_t), 4, data);
    }

    return true;
}

void AEnvironmentMap::LoadInternalResource(AStringView _Path)
{
    //uint8_t color[4] = {10, 10, 10, 255};
    uint8_t color[4] = {0, 0, 0, 255};

    ImageStorageDesc desc;

    desc.Type       = TEXTURE_CUBE;
    desc.Width      = 1;
    desc.Height     = 1;
    desc.SliceCount = 6;
    desc.NumMipmaps = 1;
    desc.Format     = TEXTURE_FORMAT_RGBA8_UNORM;
    desc.Flags      = IMAGE_STORAGE_NO_ALPHA;

    ImageStorage storage(desc);

    for (uint32_t slice = 0; slice < desc.SliceCount; ++slice)
    {
        ImageViewDesc view;
        view.FirstSlice = slice;
        view.SliceCount = 1;
        view.MipmapIndex = 0;
        storage.GetSubresource(view).Write(0, 0, 1, 1, color);
    }
    
    InitializeFromImage(storage);
}
