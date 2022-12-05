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

#include "Controller.h"
#include "HUD.h"
#include "AudioSystem.h"
#include "Terrain.h"
#include <Renderer/VT/VirtualTextureFeedback.h>

class ColorGradingParameters : public ABaseObject
{
    HK_CLASS(ColorGradingParameters, ABaseObject)

public:
    ColorGradingParameters();

    void SetLUT(ATexture* Texture);

    ATexture* GetLUT() { return m_LUT; }

    void SetGrain(Float3 const& grain);

    Float3 const& GetGrain() const { return m_Grain; }

    void SetGamma(Float3 const& gamma);

    Float3 const& GetGamma() const { return m_Gamma; }

    void SetLift(Float3 const& lift);

    Float3 const& GetLift() const { return m_Lift; }

    void SetPresaturation(Float3 const& presaturation);

    Float3 const& GetPresaturation() const { return m_Presaturation; }

    void SetTemperature(float temperature);

    float GetTemperature() const { return m_Temperature; }

    Float3 const GetTemperatureScale() const { return m_TemperatureScale; }

    void SetTemperatureStrength(Float3 const& temperatureStrength);

    Float3 const& GetTemperatureStrength() const { return m_TemperatureStrength; }

    void SetBrightnessNormalization(float brightnessNormalization);

    float GetBrightnessNormalization() const { return m_BrightnessNormalization; }

    void SetAdaptationSpeed(float adaptationSpeed);

    float GetAdaptationSpeed() const { return m_AdaptationSpeed; }

    void SetDefaults();

private:
    TRef<ATexture> m_LUT;
    Float3         m_Grain;
    Float3         m_Gamma;
    Float3         m_Lift;
    Float3         m_Presaturation;
    float          m_Temperature;
    Float3         m_TemperatureScale = Float3(1.0f);
    Float3         m_TemperatureStrength;
    float          m_BrightnessNormalization;
    float          m_AdaptationSpeed;
};

class VignetteParameters : public ABaseObject
{
    HK_CLASS(VignetteParameters, ABaseObject)

public:
    VignetteParameters() = default;
    /*
    TODO:
    void          SetColor(Float3 const& color);
    Float3 const& GetColor() const { return Color; }

    void  SetOuterRadius(float outerRadius);
    float GetOuterRadius() const { return OuterRadius; }

    void  SetInnerRadius(float innerRadius);
    float GetInnerRadius() const { return InnerRadius; }

    void  SetIntensity(float intensity);
    float GetIntensity() const { return Intensity; }
    */

    Float4 ColorIntensity = Float4(0, 0, 0, 0.4f); // rgb, intensity
    float  OuterRadiusSqr = 0.7f * 0.7f;
    float  InnerRadiusSqr = 0.6f * 0.6f;
};

class WorldRenderView : public ABaseObject
{
    HK_CLASS(WorldRenderView, ABaseObject)

public:
    Color4                       BackgroundColor  = Color4(0.3f, 0.3f, 0.8f);
    bool                         bClearBackground = false;
    bool                         bWireframe       = false;
    bool                         bDrawDebug       = false;
    VISIBILITY_GROUP             VisibilityMask   = VISIBILITY_GROUP_ALL;
    TRef<ColorGradingParameters> ColorGrading;
    TRef<VignetteParameters>     Vignette;

    //TEvent<void(RenderCore::ITexture)> E_OnTextureReady; // TODO?
    //uint32_t                           RenderingOrder{}; // TODO?

    WorldRenderView();
    ~WorldRenderView();

    void SetViewport(uint32_t width, uint32_t height);

    void SetCamera(ACameraComponent* camera);
    void SetCullingCamera(ACameraComponent* camera);

    ATexture* GetCurrentExposure() { return m_CurrentExposure; }
    ATexture* GetCurrentColorGradingLUT() { return m_CurrentColorGradingLUT; }

    ATextureView* GetTextureView();

    /*
    TODO    
    enum CUSTOM_DEPTH_STENCIL_BUFFER
    {
        CUSTOM_DEPTH_STENCIL_DISABLED,
        CUSTOM_DEPTH,
        CUSTOM_DEPTH_STENCIL
    };
    void SetCustomDepthStencil(CUSTOM_DEPTH_STENCIL_BUFFER customDepthStencil);
    CUSTOM_DEPTH_STENCIL_BUFFER GetCustomDepthStencil() const { return m_CustomDepthStencil; }
    */

    /*
    TODO
    struct PickingQueryResult
    {
        // Object id
        uint32_t Id;
        // Pick screen position X
        uint32_t X;
        // Pick screen position Y
        uint32_t Y;
        // Distance to object
        // NOTE: To reconstruct position: Float3 pos = rayStart + rayDir * Distance
        // rayStart, rayDIr can be retrived from camera by X, Y
        float Distance;
    };

    typedef void (*PickingQueryCallback)(PickingQueryResult& result);

    void PickQuery(uint32_t x, uint32_t y, PickingQueryCallback callback);
    */

    // TODO: TonemappingExposure, bTonemappingAutoExposure, TonemappingMethod:Disabled,Reinhard,Uncharted,etc
    // TODO: Wireframe color/line width
    // TODO: bBloomEnabled, BloomParams[4]
    // TODO: Render mode: Polygons,Triangles,Solid,Solid+Triangles,Solid+Polygons,etc (for editor)

private:
    class TextureViewImpl : public ATextureView
    {
    public:
        TextureViewImpl(WorldRenderView* pWorldRenderView) :
            m_WorldRenderView(pWorldRenderView)
        {}

        void SetViewport(uint32_t width, uint32_t height)
        {
            m_Width = width;
            m_Height = height;
        }

        void SetResource(RenderCore::ITexture* pResource)
        {
            m_Resource = pResource;
        }

        TRef<WorldRenderView> m_WorldRenderView;
    };

    RenderCore::ITexture* AcquireRenderTarget();
    RenderCore::ITexture* AcquireLightTexture();
    RenderCore::ITexture* AcquireDepthTexture();

    TWeakRef<ACameraComponent>              m_pCamera;
    TWeakRef<ACameraComponent>              m_pCullingCamera;
    TWeakRef<TextureViewImpl>               m_WorldViewTex;
    uint32_t                                m_Width{};
    uint32_t                                m_Height{};
    TRef<RenderCore::ITexture>              m_LightTexture;
    TRef<RenderCore::ITexture>              m_DepthTexture;
    TRef<RenderCore::ITexture>              m_RenderTarget;
    THashMap<uint64_t, class ATerrainView*> m_TerrainViews;     // TODO: Needs to be cleaned from time to time
    Float4x4                                m_ProjectionMatrix; // last rendered projection
    Float4x4                                m_ViewMatrix;       // last rendered view
    float                                   m_ScaledWidth{};
    float                                   m_ScaledHeight{};
    AVirtualTextureFeedback                 m_VTFeedback;
    TRef<ATexture>                          m_CurrentColorGradingLUT;
    TRef<ATexture>                          m_CurrentExposure;

    friend class ARenderFrontend;
};