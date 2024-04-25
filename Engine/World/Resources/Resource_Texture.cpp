#include "Resource_Texture.h"

#include <Engine/GameApplication/GameApplication.h>

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

TextureResource::TextureResource(IBinaryStreamReadInterface& stream, ResourceManager* resManager)
{
    Read(stream, resManager);
}

TextureResource::~TextureResource()
{
}

TextureResource::TextureResource(ImageStorage image) :
    m_Image(std::move(image))
{
}

bool TextureResource::Read(IBinaryStreamReadInterface& stream, ResourceManager* resManager)
{
    String const& fn = stream.GetName();

    if (GetImageFileFormat(fn) != IMAGE_FILE_FORMAT_UNKNOWN)
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

void TextureResource::Upload()
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
            Allocate1D(format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width);
            break;
        case TEXTURE_1D_ARRAY:
            Allocate1DArray(format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width, m_Image.GetDesc().SliceCount);
            break;
        case TEXTURE_2D:
            Allocate2D(format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width, m_Image.GetDesc().Height);
            break;
        case TEXTURE_2D_ARRAY:
            Allocate2DArray(format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width, m_Image.GetDesc().Height, m_Image.GetDesc().SliceCount);
            break;
        case TEXTURE_3D:
            Allocate3D(format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width, m_Image.GetDesc().Height, m_Image.GetDesc().Depth);
            break;
        case TEXTURE_CUBE:
            AllocateCubemap(format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width);
            break;
        case TEXTURE_CUBE_ARRAY:
            AllocateCubemapArray(format, m_Image.GetDesc().NumMipmaps, m_Image.GetDesc().Width, m_Image.GetDesc().SliceCount / 6);
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

void SetTextureSwizzle(TEXTURE_FORMAT const& Format, RenderCore::TextureSwizzle& _Swizzle)
{
    TextureFormatInfo const& info = GetTextureFormatInfo(Format);

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
        _Swizzle.R = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.G = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.B = RenderCore::TEXTURE_SWIZZLE_R;
        _Swizzle.A = RenderCore::TEXTURE_SWIZZLE_R;
    }
}

} // namespace

void TextureResource::Allocate1D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width)
{
    m_Type = TEXTURE_1D;
    m_Format = Format;
    m_Width = Width;
    m_Height = 1;
    m_Depth = 1;
    m_NumMipmaps = NumMipLevels;

    RenderCore::TextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::TextureResolution1D(Width));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GameApplication::GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::Allocate1DArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t ArraySize)
{
    m_Type = TEXTURE_1D_ARRAY;
    m_Format = Format;
    m_Width = Width;
    m_Height = 1;
    m_Depth = ArraySize;
    m_NumMipmaps = NumMipLevels;

    RenderCore::TextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::TextureResolution1DArray(Width, ArraySize));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GameApplication::GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::Allocate2D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height)
{
    m_Type = TEXTURE_2D;
    m_Format = Format;
    m_Width = Width;
    m_Height = Height;
    m_Depth = 1;
    m_NumMipmaps = NumMipLevels;

    RenderCore::TextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::TextureResolution2D(Width, Height));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GameApplication::GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::Allocate2DArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height, uint32_t ArraySize)
{
    m_Type = TEXTURE_2D_ARRAY;
    m_Format = Format;
    m_Width = Width;
    m_Height = Height;
    m_Depth = ArraySize;
    m_NumMipmaps = NumMipLevels;

    RenderCore::TextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::TextureResolution2DArray(Width, Height, ArraySize));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GameApplication::GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::Allocate3D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height, uint32_t Depth)
{
    m_Type = TEXTURE_3D;
    m_Format = Format;
    m_Width = Width;
    m_Height = Height;
    m_Depth = Depth;
    m_NumMipmaps = NumMipLevels;

    RenderCore::TextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::TextureResolution3D(Width, Height, Depth));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GameApplication::GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::AllocateCubemap(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width)
{
    m_Type = TEXTURE_CUBE;
    m_Format = Format;
    m_Width = Width;
    m_Height = Width;
    m_Depth = 1;
    m_NumMipmaps = NumMipLevels;

    RenderCore::TextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::TextureResolutionCubemap(Width));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GameApplication::GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

void TextureResource::AllocateCubemapArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t ArraySize)
{
    m_Type = TEXTURE_CUBE_ARRAY;
    m_Format = Format;
    m_Width = Width;
    m_Height = Width;
    m_Depth = ArraySize;
    m_NumMipmaps = NumMipLevels;

    RenderCore::TextureDesc textureDesc;
    textureDesc.SetResolution(RenderCore::TextureResolutionCubemapArray(Width, ArraySize));
    textureDesc.SetFormat(Format);
    textureDesc.SetMipLevels(NumMipLevels);
    textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE);

    SetTextureSwizzle(Format, textureDesc.Swizzle);

    GameApplication::GetRenderDevice()->CreateTexture(textureDesc, &m_TextureGPU);

    //if (m_View)
    //    m_View->SetResource(m_TextureGPU);
}

bool TextureResource::WriteData(uint32_t LocationX, uint32_t LocationY, uint32_t LocationZ, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t MipLevel, const void* pData)
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
        Depth = 1;
    }

    HK_ASSERT((LocationX % info.BlockSize) == 0);
    HK_ASSERT((LocationY % info.BlockSize) == 0);
    HK_ASSERT((Width % info.BlockSize) == 0);
    HK_ASSERT((Height % info.BlockSize) == 0);

    // TODO: bounds check?

    RenderCore::TextureRect rect;
    rect.Offset.X = LocationX;
    rect.Offset.Y = LocationY;
    rect.Offset.Z = LocationZ;
    rect.Offset.MipLevel = MipLevel;
    rect.Dimension.X = Width;
    rect.Dimension.Y = Height;
    rect.Dimension.Z = Depth;

    size_t rowWidth = Width / info.BlockSize * info.BytesPerBlock;
    size_t sizeInBytes = rowWidth * Height / info.BlockSize * Depth;

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

bool TextureResource::WriteData1D(uint32_t LocationX, uint32_t Width, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_1D && m_Type != TEXTURE_1D_ARRAY)
    {
        LOG("Texture::WriteData1D: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(LocationX, 0, 0, Width, 1, 1, MipLevel, pData);
}

bool TextureResource::WriteData1DArray(uint32_t LocationX, uint32_t Width, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_1D_ARRAY)
    {
        LOG("Texture::WriteData1DArray: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(LocationX, 0, ArrayLayer, Width, 1, 1, MipLevel, pData);
}

bool TextureResource::WriteData2D(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_2D && m_Type != TEXTURE_2D_ARRAY)
    {
        LOG("Texture::WriteData2D: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(LocationX, LocationY, 0, Width, Height, 1, MipLevel, pData);
}

bool TextureResource::WriteData2DArray(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_2D_ARRAY)
    {
        LOG("Texture::WriteData2DArray: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(LocationX, LocationY, ArrayLayer, Width, Height, 1, MipLevel, pData);
}

bool TextureResource::WriteData3D(uint32_t LocationX, uint32_t LocationY, uint32_t LocationZ, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_3D)
    {
        LOG("Texture::WriteData3D: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(LocationX, LocationY, LocationZ, Width, Height, Depth, MipLevel, pData);
}

bool TextureResource::WriteDataCubemap(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t FaceIndex, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_CUBE && m_Type != TEXTURE_CUBE_ARRAY)
    {
        LOG("Texture::WriteDataCubemap: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(LocationX, LocationY, FaceIndex, Width, Height, 1, MipLevel, pData);
}

bool TextureResource::WriteDataCubemapArray(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t FaceIndex, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData)
{
    if (m_Type != TEXTURE_CUBE_ARRAY)
    {
        LOG("Texture::WriteDataCubemapArray: called for {}\n", TextureTypeName[m_Type]);
        return false;
    }
    return WriteData(LocationX, LocationY, ArrayLayer * 6 + FaceIndex, Width, Height, 1, MipLevel, pData);
}

HK_NAMESPACE_END
