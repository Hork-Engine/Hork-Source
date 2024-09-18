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

#include "Resource_Texture.h"

HK_NAMESPACE_BEGIN

namespace
{
const char* TextureTypeName[] =
{
    "TEXTURE_1D",
    "TEXTURE_1D_ARRAY",
    "TEXTURE_2D",
    "TEXTURE_2D_ARRAY",
    "TEXTURE_3D",
    "TEXTURE_CUBE",
    "TEXTURE_CUBE_ARRAY",
};
}

TextureResource::~TextureResource()
{
}

TextureResource::TextureResource(ImageStorage image) :
    m_Image(std::move(image))
{
}

UniqueRef<TextureResource> TextureResource::sLoad(IBinaryStreamReadInterface& stream)
{
    UniqueRef<TextureResource> resource = MakeUnique<TextureResource>();
    if (!resource->Read(stream))
        return {};
    return resource;
}

bool TextureResource::Read(IBinaryStreamReadInterface& stream)
{
    if (GetImageFileFormat(stream.GetName()) != IMAGE_FILE_FORMAT_UNKNOWN)
    {
        ImageMipmapConfig mipmapGen;
        mipmapGen.EdgeMode = IMAGE_RESAMPLE_EDGE_WRAP;
        mipmapGen.Filter = IMAGE_RESAMPLE_FILTER_MITCHELL;

        m_Image = CreateImage(stream, &mipmapGen, IMAGE_STORAGE_FLAGS_DEFAULT, TEXTURE_FORMAT_UNDEFINED);
        if (!m_Image)
            return false;

        return true;
    }

    uint32_t fileMagic = stream.ReadUInt32();

    if (fileMagic != MakeResourceMagic(Type, Version))
    {
        LOG("Unexpected file format\n");
        return false;
    }

    stream.ReadObject(m_Image);

    return true;
}

//bool TextureResource::Write(IBinaryStreamWriteInterface& stream, ImageStorage const& storage)
//{
//    stream.WriteUInt32(MakeResourceMagic(Type, Version));
//    stream.WriteObject(storage);
//    return true;
//}

namespace AssetUtils
{

bool CreateTexture(IBinaryStreamWriteInterface& stream, ImageStorage const& storage)
{
    stream.WriteUInt32(MakeResourceMagic(TextureResource::Type, TextureResource::Version));
    stream.WriteObject(storage);
    return true;    
}

}

void TextureResource::Upload(RHI::IDevice* device)
{
    if (!m_Image)
    {
        LOG("TextureResource::Upload: empty image data\n");
        return;
    }

    TEXTURE_FORMAT format = m_Image.GetDesc().Format;

    switch (m_Image.GetDesc().Type)
    {
        case TEXTURE_1D:
            Allocate1D(device, format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width);
            break;
        case TEXTURE_1D_ARRAY:
            Allocate1DArray(device, format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width, m_Image.GetDesc().SliceCount);
            break;
        case TEXTURE_2D:
            Allocate2D(device, format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width, m_Image.GetDesc().Height);
            break;
        case TEXTURE_2D_ARRAY:
            Allocate2DArray(device, format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width, m_Image.GetDesc().Height, m_Image.GetDesc().SliceCount);
            break;
        case TEXTURE_3D:
            Allocate3D(device, format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width, m_Image.GetDesc().Height, m_Image.GetDesc().Depth);
            break;
        case TEXTURE_CUBE:
            AllocateCubemap(device, format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width);
            break;
        case TEXTURE_CUBE_ARRAY:
            AllocateCubemapArray(device, format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width, m_Image.GetDesc().SliceCount / 6);
            break;
        default:
            HK_ASSERT(0);
    };

    for (uint32_t slice = 0; slice < m_Image.GetDesc().SliceCount; ++slice)
    {
        for (uint32_t mip = 0; mip < m_Image.GetDesc().NumMipmaps; ++mip)
        {
            ImageSubresourceDesc desc;
            desc.SliceIndex = slice;
            desc.MipmapIndex = mip;

            ImageSubresource subresource = m_Image.GetSubresource(desc);

            WriteData(0, 0, slice, subresource.GetWidth(), subresource.GetHeight(), 1, mip, subresource.GetData());
        }
    }

    // Free image data
    m_Image.Reset();
}

namespace
{

void SetTextureSwizzle(TEXTURE_FORMAT const& format, RHI::TextureSwizzle& _Swizzle)
{
    TextureFormatInfo const& info = GetTextureFormatInfo(format);

    int numChannels = 0;
    if (info.bHasRed)
        ++numChannels;
    if (info.bHasGreen)
        ++numChannels;
    if (info.bHasBlue)
        ++numChannels;
    if (info.bHasAlpha)
        ++numChannels;

    if (numChannels == 1)
    {
        // Apply texture swizzle for single channel textures
        _Swizzle.R = RHI::TEXTURE_SWIZZLE_R;
        _Swizzle.G = RHI::TEXTURE_SWIZZLE_R;
        _Swizzle.B = RHI::TEXTURE_SWIZZLE_R;
        _Swizzle.A = RHI::TEXTURE_SWIZZLE_R;
    }
}

} // namespace

void TextureResource::Allocate1D(RHI::IDevice* device, TEXTURE_FORMAT format, uint32_t numMipLevels, uint32_t width)
{
    m_Type = TEXTURE_1D;
    m_Format = format;
    m_Width = width;
    m_Height = 1;
    m_Depth = 1;
    m_NumMipmaps = numMipLevels;

    RHI::TextureDesc textureDesc;
    textureDesc.SetResolution(RHI::TextureResolution1D(width));
    textureDesc.SetFormat(format);
    textureDesc.SetMipLevels(numMipLevels);
    textureDesc.SetBindFlags(RHI::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(format, textureDesc.Swizzle);

    device->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::Allocate1DArray(RHI::IDevice* device, TEXTURE_FORMAT format, uint32_t numMipLevels, uint32_t width, uint32_t arraySize)
{
    m_Type = TEXTURE_1D_ARRAY;
    m_Format = format;
    m_Width = width;
    m_Height = 1;
    m_Depth = arraySize;
    m_NumMipmaps = numMipLevels;

    RHI::TextureDesc textureDesc;
    textureDesc.SetResolution(RHI::TextureResolution1DArray(width, arraySize));
    textureDesc.SetFormat(format);
    textureDesc.SetMipLevels(numMipLevels);
    textureDesc.SetBindFlags(RHI::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(format, textureDesc.Swizzle);

    device->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::Allocate2D(RHI::IDevice* device, TEXTURE_FORMAT format, uint32_t numMipLevels, uint32_t width, uint32_t height)
{
    m_Type = TEXTURE_2D;
    m_Format = format;
    m_Width = width;
    m_Height = height;
    m_Depth = 1;
    m_NumMipmaps = numMipLevels;

    RHI::TextureDesc textureDesc;
    textureDesc.SetResolution(RHI::TextureResolution2D(width, height));
    textureDesc.SetFormat(format);
    textureDesc.SetMipLevels(numMipLevels);
    textureDesc.SetBindFlags(RHI::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(format, textureDesc.Swizzle);

    device->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::Allocate2DArray(RHI::IDevice* device, TEXTURE_FORMAT format, uint32_t numMipLevels, uint32_t width, uint32_t height, uint32_t arraySize)
{
    m_Type = TEXTURE_2D_ARRAY;
    m_Format = format;
    m_Width = width;
    m_Height = height;
    m_Depth = arraySize;
    m_NumMipmaps = numMipLevels;

    RHI::TextureDesc textureDesc;
    textureDesc.SetResolution(RHI::TextureResolution2DArray(width, height, arraySize));
    textureDesc.SetFormat(format);
    textureDesc.SetMipLevels(numMipLevels);
    textureDesc.SetBindFlags(RHI::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(format, textureDesc.Swizzle);

    device->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::Allocate3D(RHI::IDevice* device, TEXTURE_FORMAT format, uint32_t numMipLevels, uint32_t width, uint32_t height, uint32_t depth)
{
    m_Type = TEXTURE_3D;
    m_Format = format;
    m_Width = width;
    m_Height = height;
    m_Depth = depth;
    m_NumMipmaps = numMipLevels;

    RHI::TextureDesc textureDesc;
    textureDesc.SetResolution(RHI::TextureResolution3D(width, height, depth));
    textureDesc.SetFormat(format);
    textureDesc.SetMipLevels(numMipLevels);
    textureDesc.SetBindFlags(RHI::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(format, textureDesc.Swizzle);

    device->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::AllocateCubemap(RHI::IDevice* device, TEXTURE_FORMAT format, uint32_t numMipLevels, uint32_t width)
{
    m_Type = TEXTURE_CUBE;
    m_Format = format;
    m_Width = width;
    m_Height = width;
    m_Depth = 1;
    m_NumMipmaps = numMipLevels;

    RHI::TextureDesc textureDesc;
    textureDesc.SetResolution(RHI::TextureResolutionCubemap(width));
    textureDesc.SetFormat(format);
    textureDesc.SetMipLevels(numMipLevels);
    textureDesc.SetBindFlags(RHI::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(format, textureDesc.Swizzle);

    device->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::AllocateCubemapArray(RHI::IDevice* device, TEXTURE_FORMAT format, uint32_t numMipLevels, uint32_t width, uint32_t arraySize)
{
    m_Type = TEXTURE_CUBE_ARRAY;
    m_Format = format;
    m_Width = width;
    m_Height = width;
    m_Depth = arraySize;
    m_NumMipmaps = numMipLevels;

    RHI::TextureDesc textureDesc;
    textureDesc.SetResolution(RHI::TextureResolutionCubemapArray(width, arraySize));
    textureDesc.SetFormat(format);
    textureDesc.SetMipLevels(numMipLevels);
    textureDesc.SetBindFlags(RHI::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(format, textureDesc.Swizzle);

    device->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

bool TextureResource::WriteData(uint32_t locationX, uint32_t locationY, uint32_t locationZ, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, const void* pData)
{
    if (!m_Width)
    {
        LOG("Texture::WriteData: texture is not initialized\n");
        return false;
    }

    TextureFormatInfo const& info = GetTextureFormatInfo(m_Format);

    if (info.BlockSize > 1)
    {
        // Compressed 3D textures are not supported
        depth = 1;
    }

    HK_ASSERT((locationX % info.BlockSize) == 0);
    HK_ASSERT((locationY % info.BlockSize) == 0);
    HK_ASSERT((width % info.BlockSize) == 0);
    HK_ASSERT((height % info.BlockSize) == 0);

    // TODO: bounds check?

    RHI::TextureRect rect;
    rect.Offset.X = locationX;
    rect.Offset.Y = locationY;
    rect.Offset.Z = locationZ;
    rect.Offset.MipLevel = mipLevel;
    rect.Dimension.X = width;
    rect.Dimension.Y = height;
    rect.Dimension.Z = depth;

    size_t rowWidth = width / info.BlockSize * info.BytesPerBlock;
    size_t sizeInBytes = rowWidth * height / info.BlockSize * depth;

    int rowAlignment;
    if (IsAligned(rowWidth, 8))
        rowAlignment = 8;
    else if (IsAligned(rowWidth, 4))
        rowAlignment = 4;
    else if (IsAligned(rowWidth, 2))
        rowAlignment = 2;
    else
        rowAlignment = 1;

    m_TextureGPU->WriteRect(rect, sizeInBytes, rowAlignment, pData);

    return true;
}

bool TextureResource::WriteData1D(uint32_t locationX, uint32_t width, uint32_t mipLevel, const void* pData)
{
    if (m_Type != TEXTURE_1D && m_Type != TEXTURE_1D_ARRAY)
    {
        LOG("Texture::WriteData1D: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(locationX, 0, 0, width, 1, 1, mipLevel, pData);
}

bool TextureResource::WriteData1DArray(uint32_t locationX, uint32_t width, uint32_t arrayLayer, uint32_t mipLevel, const void* pData)
{
    if (m_Type != TEXTURE_1D_ARRAY)
    {
        LOG("Texture::WriteData1DArray: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(locationX, 0, arrayLayer, width, 1, 1, mipLevel, pData);
}

bool TextureResource::WriteData2D(uint32_t locationX, uint32_t locationY, uint32_t width, uint32_t height, uint32_t mipLevel, const void* pData)
{
    if (m_Type != TEXTURE_2D && m_Type != TEXTURE_2D_ARRAY)
    {
        LOG("Texture::WriteData2D: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(locationX, locationY, 0, width, height, 1, mipLevel, pData);
}

bool TextureResource::WriteData2DArray(uint32_t locationX, uint32_t locationY, uint32_t width, uint32_t height, uint32_t arrayLayer, uint32_t mipLevel, const void* pData)
{
    if (m_Type != TEXTURE_2D_ARRAY)
    {
        LOG("Texture::WriteData2DArray: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(locationX, locationY, arrayLayer, width, height, 1, mipLevel, pData);
}

bool TextureResource::WriteData3D(uint32_t locationX, uint32_t locationY, uint32_t locationZ, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, const void* pData)
{
    if (m_Type != TEXTURE_3D)
    {
        LOG("Texture::WriteData3D: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(locationX, locationY, locationZ, width, height, depth, mipLevel, pData);
}

bool TextureResource::WriteDataCubemap(uint32_t locationX, uint32_t locationY, uint32_t width, uint32_t height, uint32_t faceIndex, uint32_t mipLevel, const void* pData)
{
    if (m_Type != TEXTURE_CUBE && m_Type != TEXTURE_CUBE_ARRAY)
    {
        LOG("Texture::WriteDataCubemap: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(locationX, locationY, faceIndex, width, height, 1, mipLevel, pData);
}

bool TextureResource::WriteDataCubemapArray(uint32_t locationX, uint32_t locationY, uint32_t width, uint32_t height, uint32_t faceIndex, uint32_t arrayLayer, uint32_t mipLevel, const void* pData)
{
    if (m_Type != TEXTURE_CUBE_ARRAY)
    {
        LOG("Texture::WriteDataCubemapArray: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(locationX, locationY, arrayLayer * 6 + faceIndex, width, height, 1, mipLevel, pData);
}

void TextureResource::SetTextureGPU(RHI::ITexture* texture)
{
    m_TextureGPU = texture;

    if (texture)
    {
        m_Type = texture->GetDesc().Type;
        m_Format = texture->GetDesc().Format;
        m_Width = texture->GetDesc().Resolution.Width;
        m_Height = texture->GetDesc().Resolution.Height;
        m_Depth = texture->GetDesc().Resolution.SliceCount;
        m_NumMipmaps = texture->GetDesc().NumMipLevels;
    }

}

HK_NAMESPACE_END
