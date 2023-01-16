/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "WorldRenderView.h"
#include "TerrainView.h"
#include "Engine.h"

HK_NAMESPACE_BEGIN

HK_CLASS_META(WorldRenderView)
HK_CLASS_META(ColorGradingParameters)

ColorGradingParameters::ColorGradingParameters()
{
    SetDefaults();
}

void ColorGradingParameters::SetLUT(Texture* Texture)
{
    m_LUT = Texture;
}

void ColorGradingParameters::SetGrain(Float3 const& grain)
{
    m_Grain = grain;
}

void ColorGradingParameters::SetGamma(Float3 const& gamma)
{
    m_Gamma = gamma;
}

void ColorGradingParameters::SetLift(Float3 const& lift)
{
    m_Lift = lift;
}

void ColorGradingParameters::SetPresaturation(Float3 const& presaturation)
{
    m_Presaturation = presaturation;
}

void ColorGradingParameters::SetTemperature(float temperature)
{
    m_Temperature = temperature;

    Color4 color;
    color.SetTemperature(m_Temperature);

    m_TemperatureScale.X           = color.R;
    m_TemperatureScale.Y           = color.G;
    m_TemperatureScale.Z           = color.B;
}

void ColorGradingParameters::SetTemperatureStrength(Float3 const& temperatureStrength)
{
    m_TemperatureStrength = temperatureStrength;
}

void ColorGradingParameters::SetBrightnessNormalization(float brightnessNormalization)
{
    m_BrightnessNormalization = brightnessNormalization;
}

void ColorGradingParameters::SetAdaptationSpeed(float adaptationSpeed)
{
    m_AdaptationSpeed = adaptationSpeed;
}

void ColorGradingParameters::SetDefaults()
{
    m_LUT.Reset();

    m_Grain                   = Float3(0.5f);
    m_Gamma                   = Float3(0.5f);
    m_Lift                    = Float3(0.5f);
    m_Presaturation           = Float3(1.0f);
    m_TemperatureStrength     = Float3(0.0f);
    m_BrightnessNormalization = 0.0f;
    m_AdaptationSpeed         = 2;
    m_Temperature             = 6500.0f;

    Color4 color;
    color.SetTemperature(m_Temperature);
    m_TemperatureScale.X = color.R;
    m_TemperatureScale.Y = color.G;
    m_TemperatureScale.Z = color.B;
}

WorldRenderView::WorldRenderView()
{
    static Half data[16][16][16][4];
    static bool dataInit = false;
    if (!dataInit)
    {
        for (int z = 0; z < 16; z++)
        {
            for (int y = 0; y < 16; y++)
            {
                for (int x = 0; x < 16; x++)
                {
                    data[z][y][x][0] = (float)z / 15.0f * 255.0f;
                    data[z][y][x][1] = (float)y / 15.0f * 255.0f;
                    data[z][y][x][2] = (float)x / 15.0f * 255.0f;
                    data[z][y][x][3] = 255;
                }
            }
        }
        dataInit = true;
    }

    m_CurrentColorGradingLUT = Texture::Create3D(TEXTURE_FORMAT_RGBA16_FLOAT, 1, 16, 16, 16);
    m_CurrentColorGradingLUT->WriteTextureData3D(0, 0, 0, 16, 16, 16, 0, data);

    const float initialExposure[2] = {30.0f / 255.0f, 30.0f / 255.0f};

    m_CurrentExposure = Texture::Create2D(TEXTURE_FORMAT_RG32_FLOAT, 1, 1, 1);
    m_CurrentExposure->WriteTextureData2D(0, 0, 1, 1, 0, initialExposure);
}

WorldRenderView::~WorldRenderView()
{
    for (auto& it : m_TerrainViews)
    {
        it.second->RemoveRef();
    }
}

void WorldRenderView::SetViewport(uint32_t width, uint32_t height)
{
    m_Width  = width;
    m_Height = height;

    if (m_WorldViewTex)
    {
        m_WorldViewTex->SetViewport(width, height);
    }
}

void WorldRenderView::SetCamera(CameraComponent* camera)
{
    m_pCamera = camera;
}

void WorldRenderView::SetCullingCamera(CameraComponent* camera)
{
    m_pCullingCamera = camera;
}

TextureView* WorldRenderView::GetTextureView()
{
    if (m_WorldViewTex.IsExpired())
    {
        m_WorldViewTex = NewObj<TextureViewImpl>(this);
        m_WorldViewTex->SetViewport(m_Width, m_Height);

        if (m_RenderTarget)
        {
            m_WorldViewTex->SetResource(m_RenderTarget);
        }
    }

    GarbageCollector::KeepPointerAlive(m_WorldViewTex);

    return m_WorldViewTex;
}

RenderCore::ITexture* WorldRenderView::AcquireRenderTarget()
{
    if (!m_RenderTarget || (m_RenderTarget->GetWidth() != m_Width || m_RenderTarget->GetHeight() != m_Height))
    {
        RenderCore::TextureDesc textureDesc;
        textureDesc.SetResolution(RenderCore::TextureResolution2D(m_Width, m_Height));
        textureDesc.SetFormat(TEXTURE_FORMAT_SRGBA8_UNORM);
        textureDesc.SetMipLevels(1);
        textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE | RenderCore::BIND_RENDER_TARGET);

        m_RenderTarget.Reset();
        GEngine->GetRenderDevice()->CreateTexture(textureDesc, &m_RenderTarget);

        if (m_WorldViewTex)
        {
            m_WorldViewTex->SetViewport(m_Width, m_Height);
            m_WorldViewTex->SetResource(m_RenderTarget);
        }
    }

    return m_RenderTarget;
}

RenderCore::ITexture* WorldRenderView::AcquireLightTexture()
{
    if (!m_LightTexture || (m_LightTexture->GetWidth() != m_Width || m_LightTexture->GetHeight() != m_Height))
    {
        auto size    = std::max(m_Width, m_Height);
        int  numMips = 1;
        while ((size >>= 1) > 0)
            numMips++;

        RenderCore::TextureDesc textureDesc;
        textureDesc.SetResolution(RenderCore::TextureResolution2D(m_Width, m_Height));
        textureDesc.SetFormat(TEXTURE_FORMAT_R11G11B10_FLOAT);
        textureDesc.SetMipLevels(numMips);
        textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE | RenderCore::BIND_RENDER_TARGET);

        m_LightTexture.Reset();
        GEngine->GetRenderDevice()->CreateTexture(textureDesc, &m_LightTexture);

        RenderCore::ClearValue clearVal;
        memset(&clearVal.Float4, 0, sizeof(clearVal.Float4));
        GEngine->GetRenderDevice()->GetImmediateContext()->ClearTexture(m_LightTexture, 0, RenderCore::FORMAT_FLOAT4, &clearVal);
    }

    return m_LightTexture;
}

RenderCore::ITexture* WorldRenderView::AcquireDepthTexture()
{
    if (!m_DepthTexture || (m_DepthTexture->GetWidth() != m_Width || m_DepthTexture->GetHeight() != m_Height))
    {
        RenderCore::TextureDesc textureDesc;
        textureDesc.SetResolution(RenderCore::TextureResolution2D(m_Width, m_Height));
        textureDesc.SetFormat(TEXTURE_FORMAT_R32_FLOAT);
        textureDesc.SetMipLevels(1);
        textureDesc.SetBindFlags(RenderCore::BIND_SHADER_RESOURCE /* | RenderCore::BIND_RENDER_TARGET*/);

        m_DepthTexture.Reset();
        GEngine->GetRenderDevice()->CreateTexture(textureDesc, &m_DepthTexture);
    }

    return m_DepthTexture;
}

RenderCore::ITexture* WorldRenderView::AcquireHBAOMaps()
{
    if (bAllowHBAO)
    {
        uint32_t  width         = ((m_Width + 3) / 4);
        uint32_t  height        = ((m_Height + 3) / 4);
        const int hbaoMapsCount = 16;

        if (!m_HBAOMaps || (m_HBAOMaps->GetWidth() != width || m_HBAOMaps->GetHeight() != height))
        {
            m_HBAOMaps.Reset();
            GEngine->GetRenderDevice()->CreateTexture(
                RenderCore::TextureDesc()
                    .SetFormat(TEXTURE_FORMAT_R32_FLOAT)
                    .SetResolution(RenderCore::TextureResolution2DArray(width, height, hbaoMapsCount))
                    .SetBindFlags(RenderCore::BIND_SHADER_RESOURCE | RenderCore::BIND_RENDER_TARGET),
                &m_HBAOMaps);
        }
    }
    else
    {
        m_HBAOMaps.Reset();
    }

    return m_HBAOMaps;
}

void WorldRenderView::ReleaseHBAOMaps()
{
    m_HBAOMaps.Reset();
}

HK_NAMESPACE_END
