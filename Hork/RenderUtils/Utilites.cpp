/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "Utilites.h"

#include "IrradianceGenerator.h"
#include "EnvProbeGenerator.h"
#include "AtmosphereRenderer.h"

#include <Hork/Image/ImageEncoders.h>

HK_NAMESPACE_BEGIN

namespace RenderUtils
{

void GenerateIrradianceMap(RHI::IDevice* device, RHI::ITexture* cubemap, Ref<RHI::ITexture>* ppTexture)
{
    SphereMesh sphereMesh(device);
    IrradianceGenerator irradianceGenerator(device, &sphereMesh);
    irradianceGenerator.Generate(cubemap, ppTexture);
}

void GenerateReflectionMap(RHI::IDevice* device, RHI::ITexture* cubemap, Ref<RHI::ITexture>* ppTexture)
{
    SphereMesh sphereMesh(device);
    EnvProbeGenerator envProbeGenerator(device, &sphereMesh);
    envProbeGenerator.Generate(7, cubemap, ppTexture);
}

void GenerateSkybox(RHI::IDevice* device, TEXTURE_FORMAT format, uint32_t resolution, Float3 const& lightDir, Ref<RHI::ITexture>* ppTexture)
{
    SphereMesh sphereMesh(device);
    AtmosphereRenderer atmosphereRenderer(device, &sphereMesh);
    atmosphereRenderer.Render(format, resolution, lightDir, ppTexture);
}

bool GenerateAndSaveEnvironmentMap(RHI::IDevice* device, ImageStorage const& skyboxImage, StringView envmapFile)
{
    Ref<RHI::ITexture> sourceMap, irradianceMap, reflectionMap;

    if (!skyboxImage || skyboxImage.GetDesc().Type != TEXTURE_CUBE)
    {
        LOG("GenerateAndSaveEnvironmentMap: invalid skybox\n");
        return false;
    }

    int width = skyboxImage.GetDesc().Width;

    RHI::TextureDesc textureDesc;
    textureDesc.SetResolution(RHI::TextureResolutionCubemap(width));
    textureDesc.SetFormat(skyboxImage.GetDesc().Format);
    textureDesc.SetMipLevels(1);
    textureDesc.SetBindFlags(RHI::BIND_SHADER_RESOURCE);

    if (skyboxImage.NumChannels() == 1)
    {
        // Apply texture swizzle for single channel textures
        textureDesc.Swizzle.R = RHI::TEXTURE_SWIZZLE_R;
        textureDesc.Swizzle.G = RHI::TEXTURE_SWIZZLE_R;
        textureDesc.Swizzle.B = RHI::TEXTURE_SWIZZLE_R;
        textureDesc.Swizzle.A = RHI::TEXTURE_SWIZZLE_R;
    }

    device->CreateTexture(textureDesc, &sourceMap);

    RHI::TextureRect rect;
    rect.Offset.X        = 0;
    rect.Offset.Y        = 0;
    rect.Offset.MipLevel = 0;
    rect.Dimension.X     = width;
    rect.Dimension.Y     = width;
    rect.Dimension.Z     = 1;

    ImageSubresourceDesc subresDesc;
    subresDesc.MipmapIndex = 0;

    for (int faceNum = 0; faceNum < 6; faceNum++)
    {
        rect.Offset.Z = faceNum;

        subresDesc.SliceIndex = faceNum;

        ImageSubresource subresouce = skyboxImage.GetSubresource(subresDesc);

        sourceMap->WriteRect(rect, subresouce.GetSizeInBytes(), 1, subresouce.GetData());
    }

    GenerateIrradianceMap(device, sourceMap, &irradianceMap);
    GenerateReflectionMap(device, sourceMap, &reflectionMap);

    // Preform some validation
    HK_ASSERT(irradianceMap->GetDesc().Resolution.Width == irradianceMap->GetDesc().Resolution.Height);
    HK_ASSERT(reflectionMap->GetDesc().Resolution.Width == reflectionMap->GetDesc().Resolution.Height);
    HK_ASSERT(irradianceMap->GetDesc().Format == TEXTURE_FORMAT_R11G11B10_FLOAT);
    HK_ASSERT(reflectionMap->GetDesc().Format == TEXTURE_FORMAT_R11G11B10_FLOAT);

    File f = File::sOpenWrite(envmapFile);
    if (!f)
    {
        LOG("Failed to write {}\n", envmapFile);
        return false;
    }

    constexpr uint32_t ASSET_ENVMAP = 8;
    constexpr uint32_t ASSET_VERSION_ENVMAP = 2;

    f.WriteUInt32(ASSET_ENVMAP);
    f.WriteUInt32(ASSET_VERSION_ENVMAP);
    f.WriteUInt32(irradianceMap->GetWidth());
    f.WriteUInt32(reflectionMap->GetWidth());

    // Choose max width for memory allocation
    int maxSize = Math::Max(irradianceMap->GetWidth(), reflectionMap->GetWidth());

    Vector<uint32_t> buffer(maxSize * maxSize * 6);

    uint32_t* data = buffer.ToPtr();

    int numPixels = irradianceMap->GetWidth() * irradianceMap->GetWidth() * 6;
    irradianceMap->Read(0, numPixels * sizeof(uint32_t), 4, data);

    f.WriteWords<uint32_t>(data, numPixels);

    for (int mipLevel = 0; mipLevel < reflectionMap->GetDesc().NumMipLevels; mipLevel++)
    {
        int mipWidth = reflectionMap->GetWidth() >> mipLevel;
        HK_ASSERT(mipWidth > 0);

        numPixels = mipWidth * mipWidth * 6;

        reflectionMap->Read(mipLevel, numPixels * sizeof(uint32_t), 4, data);

        f.WriteWords<uint32_t>(data, numPixels);
    }
    return true;
}

bool GenerateAndSaveEnvironmentMap(RHI::IDevice* device, SkyboxImportSettings const& importSettings, StringView envmapFile)
{
    ImageStorage image = LoadSkyboxImages(importSettings);

    if (!image)
    {
        return false;
    }

    return GenerateAndSaveEnvironmentMap(device, image, envmapFile);
}

ImageStorage GenerateAtmosphereSkybox(RHI::IDevice* device, SKYBOX_IMPORT_TEXTURE_FORMAT format, uint32_t resolution, Float3 const& lightDir)
{
    TEXTURE_FORMAT renderFormat;

    switch (format)
    {
        case SKYBOX_IMPORT_TEXTURE_FORMAT_SRGBA8_UNORM:
        case SKYBOX_IMPORT_TEXTURE_FORMAT_BC1_UNORM_SRGB:
            renderFormat = TEXTURE_FORMAT_SRGBA8_UNORM;
            break;
        case SKYBOX_IMPORT_TEXTURE_FORMAT_SBGRA8_UNORM:
            renderFormat = TEXTURE_FORMAT_SBGRA8_UNORM;
            break;
        case SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT:
            renderFormat = TEXTURE_FORMAT_R11G11B10_FLOAT;
            break;
        case SKYBOX_IMPORT_TEXTURE_FORMAT_BC6H_UFLOAT:
            renderFormat = TEXTURE_FORMAT_RGBA32_FLOAT;
            break;
        default:
            LOG("GenerateAtmosphereSkybox: unexpected texture format\n");
            return {};
    }

    TextureFormatInfo const& info = GetTextureFormatInfo((TEXTURE_FORMAT)format);

    if (resolution % info.BlockSize)
    {
        LOG("GenerateAtmosphereSkybox: skybox resolution must be block aligned\n");
        return {};
    }

    Ref<RHI::ITexture> skybox;
    GenerateSkybox(device, renderFormat, resolution, lightDir, &skybox);

    RHI::TextureRect rect;
    rect.Offset.X        = 0;
    rect.Offset.Y        = 0;
    rect.Offset.MipLevel = 0;
    rect.Dimension.X     = resolution;
    rect.Dimension.Y     = resolution;
    rect.Dimension.Z     = 1;

    ImageStorageDesc desc;
    desc.Type       = TEXTURE_CUBE;
    desc.Width      = resolution;
    desc.Height     = resolution;
    desc.SliceCount = 6;
    desc.NumMipmaps = 1;
    desc.Format     = (TEXTURE_FORMAT)format;
    desc.Flags      = IMAGE_STORAGE_NO_ALPHA;

    ImageStorage storage(desc);

    HeapBlob temp;

    for (uint32_t faceNum = 0; faceNum < 6; faceNum++)
    {
        ImageSubresourceDesc subresDesc;
        subresDesc.SliceIndex  = faceNum;
        subresDesc.MipmapIndex = 0;

        ImageSubresource subresource = storage.GetSubresource(subresDesc);

        rect.Offset.Z = faceNum;

        switch (format)
        {
            case SKYBOX_IMPORT_TEXTURE_FORMAT_SRGBA8_UNORM:
            case SKYBOX_IMPORT_TEXTURE_FORMAT_SBGRA8_UNORM:
            case SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT:
                skybox->ReadRect(rect, subresource.GetSizeInBytes(), 4, subresource.GetData());
                break;
            case SKYBOX_IMPORT_TEXTURE_FORMAT_BC1_UNORM_SRGB:
                if (!temp)
                    temp.Reset(resolution * resolution * 4);

                skybox->ReadRect(rect, temp.Size(), 4, temp.GetData());
                TextureBlockCompression::CompressBC1(temp.GetData(), subresource.GetData(), resolution, resolution);
                break;
            case SKYBOX_IMPORT_TEXTURE_FORMAT_BC6H_UFLOAT:
                if (!temp)
                    temp.Reset(resolution * resolution * 4 * sizeof(float));

                skybox->ReadRect(rect, temp.Size(), 4, temp.GetData());
                TextureBlockCompression::CompressBC6h(temp.GetData(), subresource.GetData(), resolution, resolution, false);
                break;
            default:
                HK_ASSERT(0);
        }
    }
    return storage;
}

}

HK_NAMESPACE_END
