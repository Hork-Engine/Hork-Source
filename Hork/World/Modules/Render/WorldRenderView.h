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

#include <Hork/Renderer/VT/VirtualTextureFeedback.h>
#include <Hork/Resources/ResourceManager.h>
#include <Hork/Resources/Resource_Texture.h>
#include <Hork/Resources/Resource_Terrain.h>
#include <Hork/World/World.h>
#include <Hork/World/Modules/Render/Components/CameraComponent.h>

#include "VisibilitySystem.h"

HK_NAMESPACE_BEGIN

class World;

class ColorGradingParameters : public RefCounted
{
public:
    ColorGradingParameters();

    void SetLUT(TextureHandle Texture);

    TextureHandle GetLUT() { return m_LUT; }

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
    TextureHandle m_LUT;
    Float3        m_Grain;
    Float3        m_Gamma;
    Float3        m_Lift;
    Float3        m_Presaturation;
    float         m_Temperature;
    Float3        m_TemperatureScale = Float3(1.0f);
    Float3        m_TemperatureStrength;
    float         m_BrightnessNormalization;
    float         m_AdaptationSpeed;
};

class VignetteParameters : public RefCounted
{
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
    float  OuterRadiusSqr = Math::Square(0.7f);
    float  InnerRadiusSqr = Math::Square(0.6f);
};

class WorldRenderView : public RefCounted
{
public:
    Color4                      BackgroundColor  = Color4(0.3f, 0.3f, 0.8f);
    bool                        bClearBackground = false;
    bool                        bWireframe       = false;
    bool                        bDrawDebug       = false;
    bool                        bAllowHBAO       = true;
    bool                        bAllowMotionBlur = true;
    ANTIALIASING_TYPE           AntialiasingType = ANTIALIASING_SMAA;
    Ref<ColorGradingParameters> ColorGrading;
    Ref<VignetteParameters>     Vignette;

    //uint32_t                  RenderingOrder{}; // TODO?

                            WorldRenderView();
                            ~WorldRenderView();

    void                    SetViewport(uint32_t width, uint32_t height);

    uint32_t                GetWidth() const { return m_Width; }
    uint32_t                GetHeight() const { return m_Height; }

    void                    SetWorld(World* world);
    World*                  GetWorld() { return m_World; }

    void                    SetCamera(Handle32<CameraComponent> camera);
    Handle32<CameraComponent> GetCamera() const { return m_Camera; }

    void                    SetCullingCamera(Handle32<CameraComponent> camera);
    Handle32<CameraComponent> GetCullingCamera() const { return m_CullingCamera; }

    RenderCore::ITexture*   GetCurrentExposure() { return m_CurrentExposure; }
    RenderCore::ITexture*   GetCurrentColorGradingLUT() { return m_CurrentColorGradingLUT; }

    TextureHandle           GetTextureHandle();

    class TerrainView*      GetTerrainView(TerrainHandle resource);

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

    RenderCore::ITexture* AcquireRenderTarget();

private:
    RenderCore::ITexture* AcquireLightTexture();
    RenderCore::ITexture* AcquireDepthTexture();
    RenderCore::ITexture* AcquireHBAOMaps();
    void                  ReleaseHBAOMaps();

    Handle32<CameraComponent>           m_Camera;
    Handle32<CameraComponent>           m_CullingCamera;
    World*                              m_World{}; // TODO: refcounting or handles

    uint32_t                            m_Width{};
    uint32_t                            m_Height{};
    Ref<RenderCore::ITexture>           m_LightTexture;
    Ref<RenderCore::ITexture>           m_DepthTexture;
    Ref<RenderCore::ITexture>           m_HBAOMaps;
    HashMap<ResourceID, TerrainView*>   m_TerrainViews;     // TODO: Needs to be cleaned from time to time
    Float4x4                            m_ProjectionMatrix; // last rendered projection
    Float4x4                            m_ViewMatrix;       // last rendered view
    float                               m_ScaledWidth{};
    float                               m_ScaledHeight{};
    VirtualTextureFeedback              m_VTFeedback;
    Ref<RenderCore::ITexture>           m_CurrentColorGradingLUT;
    Ref<RenderCore::ITexture>           m_CurrentExposure;
    int                                 m_FrameNum{};
    TextureHandle                       m_HandleRT;

    friend class RenderFrontend;
};

HK_NAMESPACE_END
