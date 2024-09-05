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

#pragma once

#include "ResourceHandle.h"
#include "ResourceBase.h"

#include <Engine/Core/BinaryStream.h>
#include <Engine/Image/Image.h>
#include <Engine/RenderCore/Texture.h>

HK_NAMESPACE_BEGIN

class TextureResource : public ResourceBase
{
public:
    static const uint8_t        Type = RESOURCE_TEXTURE;
    static const uint8_t        Version = 1;

                                TextureResource() = default;
    explicit                    TextureResource(ImageStorage image);
                                ~TextureResource();

    static UniqueRef<TextureResource> sLoad(IBinaryStreamReadInterface& stream);
    //static bool                 Write(IBinaryStreamWriteInterface& stream, ImageStorage const& storage);

    bool                        Read(IBinaryStreamReadInterface& stream);

    void                        Upload() override;

    /// Allocate empty 1D texture
    void                        Allocate1D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width);

    /// Allocate empty 1D array texture
    void                        Allocate1DArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t ArraySize);

    /// Allocate empty 2D texture
    void                        Allocate2D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height);

    /// Allocate empty 2D array texture
    void                        Allocate2DArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height, uint32_t ArraySize);

    /// Allocate empty 3D texture
    void                        Allocate3D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height, uint32_t Depth);

    /// Allocate empty cubemap texture
    void                        AllocateCubemap(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width);

    /// Allocate empty cubemap array texture
    void                        AllocateCubemapArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t ArraySize);

    /// Fill texture data for any texture type.
    bool                        WriteData(uint32_t LocationX, uint32_t LocationY, uint32_t LocationZ, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t MipLevel, const void* pData);

    /// Helper. Fill texture data.
    bool                        WriteData1D(uint32_t LocationX, uint32_t Width, uint32_t MipLevel, const void* pData);

    /// Helper. Fill texture data.
    bool                        WriteData1DArray(uint32_t LocationX, uint32_t Width, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData);

    /// Helper. Fill texture data.
    bool                        WriteData2D(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t MipLevel, const void* pData);

    /// Helper. Fill texture data.
    bool                        WriteData2DArray(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData);

    /// Helper. Fill texture data.
    bool                        WriteData3D(uint32_t LocationX, uint32_t LocationY, uint32_t LocationZ, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t MipLevel, const void* pData);

    /// Helper. Fill texture data.
    bool                        WriteDataCubemap(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t FaceIndex, uint32_t MipLevel, const void* pData);

    /// Helper. Fill texture data.
    bool                        WriteDataCubemapArray(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t FaceIndex, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData);

    void                        SetTextureGPU(RenderCore::ITexture* texture);
    RenderCore::ITexture*       GetTextureGPU() { return m_TextureGPU; }

    TEXTURE_TYPE                GetType() const { return m_Type; }
    TEXTURE_FORMAT              GetFormat() const { return m_Format; }
    uint32_t                    GetWidth() const { return m_Width; }
    uint32_t                    GetHeight() const { return m_Height; }
    uint32_t                    GetDepth() const { return m_Depth; }
    uint32_t                    GetNumMipmaps() const { return m_NumMipmaps; }

private:
    ImageStorage                m_Image;

    Ref<RenderCore::ITexture>   m_TextureGPU;
    TEXTURE_TYPE                m_Type = TEXTURE_2D;
    TEXTURE_FORMAT              m_Format = TEXTURE_FORMAT_BGRA8_UNORM;
    uint32_t                    m_Width = 0;
    uint32_t                    m_Height = 0;
    uint32_t                    m_Depth = 0;
    uint32_t                    m_NumMipmaps = 0;
};

using TextureHandle = ResourceHandle<TextureResource>;

namespace AssetUtils
{

bool CreateTexture(IBinaryStreamWriteInterface& stream, ImageStorage const& storage);

}

HK_NAMESPACE_END
