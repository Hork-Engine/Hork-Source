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

#pragma once

#include "Resource.h"
#include <Renderer/RenderDefs.h>

class ImageStorage;

struct SColorGradingPreset
{
    Float3 Gain;
    Float3 Gamma;
    Float3 Lift;
    Float3 Presaturation;
    Float3 ColorTemperatureStrength;
    float  ColorTemperature; // in K
    float  ColorTemperatureBrightnessNormalization;
};

class ATextureView : public ABaseObject
{
    HK_CLASS(ATextureView, ABaseObject)

public:
    ATextureView() = default;

    RenderCore::ITexture* GetResource()
    {
        return m_Resource;
    }

    uint32_t GetWidth() const
    {
        return m_Width;
    }

    uint32_t GetHeight() const
    {
        return m_Height;
    }

protected:
    RenderCore::ITexture* m_Resource{};
    uint32_t              m_Width{};
    uint32_t              m_Height{};
};

/**

ATexture

*/
class ATexture : public AResource
{
    HK_CLASS(ATexture, AResource)

public:
    ATexture() = default;
    ~ATexture() = default;

    static ATexture* Create1D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width)
    {
        ATexture* texture = CreateInstanceOf<ATexture>();
        texture->Initialize1D(Format, NumMipLevels, Width);
        return texture;
    }

    static ATexture* Create1DArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t ArraySize)
    {
        ATexture* texture = CreateInstanceOf<ATexture>();
        texture->Initialize1DArray(Format, NumMipLevels, Width, ArraySize);
        return texture;
    }

    static ATexture* Create2D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height)
    {
        ATexture* texture = CreateInstanceOf<ATexture>();
        texture->Initialize2D(Format, NumMipLevels, Width, Height);
        return texture;
    }

    static ATexture* Create2DArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height, uint32_t ArraySize)
    {
        ATexture* texture = CreateInstanceOf<ATexture>();
        texture->Initialize2DArray(Format, NumMipLevels, Width, Height, ArraySize);
        return texture;
    }

    static ATexture* Create3D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height, uint32_t Depth)
    {
        ATexture* texture = CreateInstanceOf<ATexture>();
        texture->Initialize3D(Format, NumMipLevels, Width, Height, Depth);
        return texture;
    }

    static ATexture* CreateCubemap(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width)
    {
        ATexture* texture = CreateInstanceOf<ATexture>();
        texture->InitializeCubemap(Format, NumMipLevels, Width);
        return texture;
    }

    static ATexture* CreateCubemapArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t ArraySize)
    {
        ATexture* texture = CreateInstanceOf<ATexture>();
        texture->InitializeCubemapArray(Format, NumMipLevels, Width, ArraySize);
        return texture;
    }

    static ATexture* CreateFromImage(ImageStorage const& Image)
    {
        ATexture* texture = CreateInstanceOf<ATexture>();
        texture->InitializeFromImage(Image);
        return texture;
    }

    static ATexture* CreateColorGradingLUT(IBinaryStreamReadInterface& Stream)
    {
        ATexture* texture = CreateInstanceOf<ATexture>();
        texture->InitializeColorGradingLUT(Stream);
        return texture;
    }

    static ATexture* CreateColorGradingLUT(SColorGradingPreset const& Preset)
    {
        ATexture* texture = CreateInstanceOf<ATexture>();
        texture->InitializeColorGradingLUT(Preset);
        return texture;
    }

    ATextureView* GetView()
    {
        if (m_View.IsExpired())
        {
            m_View = CreateInstanceOf<TextureViewImpl>(this);
            m_View->SetResource(m_TextureGPU);
        }
        return m_View;
    }

    /** Fill texture data for any texture type. */
    bool WriteArbitraryData(uint32_t LocationX, uint32_t LocationY, uint32_t LocationZ, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t MipLevel, const void* pData);

    /** Helper. Fill texture data. */
    bool WriteTextureData1D(uint32_t LocationX, uint32_t Width, uint32_t MipLevel, const void* pData);

    /** Helper. Fill texture data. */
    bool WriteTextureData1DArray(uint32_t LocationX, uint32_t Width, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData);

    /** Helper. Fill texture data. */
    bool WriteTextureData2D(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t MipLevel, const void* pData);

    /** Helper. Fill texture data. */
    bool WriteTextureData2DArray(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData);

    /** Helper. Fill texture data. */
    bool WriteTextureData3D(uint32_t LocationX, uint32_t LocationY, uint32_t LocationZ, uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t MipLevel, const void* pData);

    /** Helper. Fill texture data. */
    bool WriteTextureDataCubemap(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t FaceIndex, uint32_t MipLevel, const void* pData);

    /** Helper. Fill texture data. */
    bool WriteTextureDataCubemapArray(uint32_t LocationX, uint32_t LocationY, uint32_t Width, uint32_t Height, uint32_t FaceIndex, uint32_t ArrayLayer, uint32_t MipLevel, const void* pData);

    TEXTURE_TYPE GetType() const { return m_Type; }

    TEXTURE_FORMAT const& GetFormat() const { return m_Format; }

    uint32_t GetDimensionX() const { return m_Width; }

    uint32_t GetDimensionY() const { return m_Height; }

    uint32_t GetDimensionZ() const { return m_Depth; }

    uint32_t GetArraySize() const;

    uint32_t GetNumMipLevels() const { return m_NumMipmaps; }

    bool IsCubemap() const;

    RenderCore::ITexture* GetGPUResource() { return m_TextureGPU; }

    void SetDebugName(AStringView DebugName);

protected:
    /** Create empty 1D texture */
    void Initialize1D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width);

    /** Create empty 1D array texture */
    void Initialize1DArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t ArraySize);

    /** Create empty 2D texture */
    void Initialize2D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height);

    /** Create texture from image */
    bool InitializeFromImage(ImageStorage const& Image);

    /** Create empty 2D array texture */
    void Initialize2DArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height, uint32_t ArraySize);

    /** Create empty 3D texture */
    void Initialize3D(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t Height, uint32_t Depth);

    /** Create 3D color grading LUT */
    void InitializeColorGradingLUT(IBinaryStreamReadInterface& Stream);

    /** Create 3D color grading LUT */
    void InitializeColorGradingLUT(SColorGradingPreset const& Preset);

    /** Create empty cubemap texture */
    void InitializeCubemap(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width);

    /** Create empty cubemap array texture */
    void InitializeCubemapArray(TEXTURE_FORMAT Format, uint32_t NumMipLevels, uint32_t Width, uint32_t ArraySize);

    /** Load resource from file */
    bool LoadResource(IBinaryStreamReadInterface& Stream) override;

    /** Create internal resource */
    void LoadInternalResource(AStringView Path) override;

    const char* GetDefaultResourcePath() const override { return "/Default/Textures/Default2D"; }

    TRef<RenderCore::ITexture> m_TextureGPU;
    TEXTURE_TYPE               m_Type       = TEXTURE_2D;
    TEXTURE_FORMAT             m_Format     = TEXTURE_FORMAT_BGRA8_UNORM;
    uint32_t                   m_Width      = 0;
    uint32_t                   m_Height     = 0;
    uint32_t                   m_Depth      = 0;
    uint32_t                   m_NumMipmaps = 0;

    class TextureViewImpl : public ATextureView
    {
        HK_CLASS(TextureViewImpl, ATextureView)

    public:
        TextureViewImpl() = default; // NOTE: It's really not needed, but the current reflection system requires this constructor.

        TextureViewImpl(ATexture* pTexture) :
            m_Texture(pTexture)
        {
            m_Width  = pTexture->GetDimensionX();
            m_Height = pTexture->GetDimensionY();
        }

        void SetResource(RenderCore::ITexture* pResource)
        {
            m_Resource = pResource;
        }

        TRef<ATexture> m_Texture;
    };
    TWeakRef<TextureViewImpl> m_View;
};
