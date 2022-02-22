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
#include "WViewport.h"
#include "Terrain.h"
#include <Renderer/VT/VirtualTextureFeedback.h> // TODO: Remove this!

#include <unordered_map>

class AInputMappings;

class ARenderingParameters : public ABaseObject
{
    HK_CLASS(ARenderingParameters, ABaseObject)

public:
    Color4 BackgroundColor = Color4(0.3f, 0.3f, 0.8f);

    bool bClearBackground = false;

    bool bWireframe = false;

    bool bDrawDebug = false;

    bool   bVignetteEnabled       = false;
    Float4 VignetteColorIntensity = Float4(0, 0, 0, 0.4f); // rgb, intensity
    float  VignetteOuterRadiusSqr = 0.7f * 0.7f;
    float  VignetteInnerRadiusSqr = 0.6f * 0.6f;
    VISIBILITY_GROUP VisibilityMask         = VISIBILITY_GROUP_ALL;

    std::unordered_map<uint64_t, class ATerrainView*> TerrainViews; // TODO: Needs to be cleaned from time to time

    // TODO: TonemappingExposure, bTonemappingAutoExposure, TonemappingMethod:Disabled,Reinhard,Uncharted,etc
    // TODO: Wireframe color/line width
    // TODO: bBloomEnabled, BloomParams[4]
    // TODO: Render mode: Polygons,Triangles,Solid,Solid+Triangles,Solid+Polygons,etc (for editor)

    ARenderingParameters();
    ~ARenderingParameters();

    ATexture* GetCurrentExposure() { return CurrentExposure; }

    void SetColorGradingEnabled(bool _ColorGradingEnabled);

    bool IsColorGradingEnabled() const { return bColorGradingEnabled; }

    void SetColorGradingLUT(ATexture* Texture);

    ATexture* GetColorGradingLUT() { return ColorGradingLUT; }

    ATexture* GetCurrentColorGradingLUT() { return CurrentColorGradingLUT; }

    void SetColorGradingGrain(Float3 const& _ColorGradingGrain);

    Float3 const& GetColorGradingGrain() const { return ColorGradingGrain; }

    void SetColorGradingGamma(Float3 const& _ColorGradingGamma);

    Float3 const& GetColorGradingGamma() const { return ColorGradingGamma; }

    void SetColorGradingLift(Float3 const& _ColorGradingLift);

    Float3 const& GetColorGradingLift() const { return ColorGradingLift; }

    void SetColorGradingPresaturation(Float3 const& _ColorGradingPresaturation);

    Float3 const& GetColorGradingPresaturation() const { return ColorGradingPresaturation; }

    void SetColorGradingTemperature(float _ColorGradingTemperature);

    float GetColorGradingTemperature() const { return ColorGradingTemperature; }

    Float3 const GetColorGradingTemperatureScale() const { return ColorGradingTemperatureScale; }

    void SetColorGradingTemperatureStrength(Float3 const& _ColorGradingTemperatureStrength);

    Float3 const& GetColorGradingTemperatureStrength() const { return ColorGradingTemperatureStrength; }

    void SetColorGradingBrightnessNormalization(float _ColorGradingBrightnessNormalization);

    float GetColorGradingBrightnessNormalization() const { return ColorGradingBrightnessNormalization; }

    void SetColorGradingAdaptationSpeed(float _ColorGradingAdaptationSpeed);

    float GetColorGradingAdaptationSpeed() const { return ColorGradingAdaptationSpeed; }

    void SetColorGradingDefaults();

    // TODO:
    //    void SetVignetteEnabled( bool _VignetteEnabled );
    //    bool IsVignetteEnabled() const { return VignetteEnabled; }

    //    void SetVignetteColor( const Float3 & _VignetteColor );
    //    const Float3 & GetVignetteColor() const { return VignetteColor; }

    //    void SetVignetteOuterRadius( const float & _VignetteOuterRadius );
    //    const Float & GetVignetteOuterRadius() const { return VignetteOuterRadius; }

    //    void SetVignetteInnerRadius( const float & _VignetteInnerRadius );
    //    const Float & GetVignetteInnerRadius() const { return VignetteInnerRadius; }

    //    void SetVignetteIntensity( const float & _VignetteIntensity );
    //    const Float & GetVignetteIntensity() const { return VignetteIntensity; }
    //    enum ECustomDepthStencilBuffer {
    //        RT_NoCustomDepth,
    //        RT_CustomDepth,
    //        RT_CustomDepthStencil
    //    };
    //   void SetCustomDepthStencil( ECustomDepthStencilBuffer _CustomDepthStencil );
    //   ECustomDepthStencilBuffer GetCustomDepthStencil() const { return CustomDepthStencil; }

    // Internal (experimental)
    Float4x4 ProjectionMatrix; // last rendered projection
    Float4x4 ViewMatrix;       // last rendered view

    AVirtualTextureFeedback VTFeedback;

    ATexture* GetLightTexture() { return LightTexture; }
    ATexture* GetDepthTexture() { return DepthTexture; }

private:
    TRef<ATexture> ColorGradingLUT;
    TRef<ATexture> CurrentColorGradingLUT;
    TRef<ATexture> CurrentExposure;
    TRef<ATexture> LightTexture;
    TRef<ATexture> DepthTexture;
    Float3         ColorGradingGrain;
    Float3         ColorGradingGamma;
    Float3         ColorGradingLift;
    Float3         ColorGradingPresaturation;
    float          ColorGradingTemperature;
    Float3         ColorGradingTemperatureScale = Float3(1.0f);
    Float3         ColorGradingTemperatureStrength;
    float          ColorGradingBrightnessNormalization;
    float          ColorGradingAdaptationSpeed;
    bool           bColorGradingEnabled;
};

class AAudioParameters : public ABaseObject
{
    HK_CLASS(AAudioParameters, ABaseObject)

public:
    //Float3 Velocity;

    //float DopplerFactor = 1;
    //float DopplerVelocity = 1;
    //float SpeedOfSound = 343.3f;
    //EAudioDistanceModel DistanceModel = AUDIO_DIST_LINEAR_CLAMPED;//AUDIO_DIST_INVERSE_CLAMPED;
    float Volume = 1.0f;

    uint32_t ListenerMask = ~0u;

    AAudioParameters() {}
    ~AAudioParameters() {}
};

/**

APlayerController

Base class for players

*/
class APlayerController : public AController
{
    HK_ACTOR(APlayerController, AController)

public:
    /** Player viewport. Don't change directly, use WViewport::SetPlayerController to attach controller to viewport. */
    TWeakRef<WViewport> Viewport;

    /** Override listener location. If listener is not specified, pawn camera will be used. */
    void SetAudioListener(ASceneComponent* _AudioListener);

    /** Set HUD actor */
    void SetHUD(AHUD* _HUD);

    /** Set viewport rendering parameters */
    void SetRenderingParameters(ARenderingParameters* _RP);

    /** Set audio rendering parameters */
    void SetAudioParameters(AAudioParameters* _AudioParameters);

    /** Set input mappings for input component */
    void SetInputMappings(AInputMappings* _InputMappings);

    /** Set controller player index */
    void SetPlayerIndex(int _ControllerId);

    /** Set player controller as primary audio listener */
    void SetCurrentAudioListener();

    float GetViewportAspectRatio() const;

    int GetViewportWidth() const { return ViewportWidth; }
    int GetViewportHeight() const { return ViewportHeight; }

    /** Get current audio listener */
    ASceneComponent* GetAudioListener();

    /** Get HUD actor */
    AHUD* GetHUD() const { return HUD; }

    /** Get viewport rendering parameters */
    ARenderingParameters* GetRenderingParameters() { return RenderingParameters; }

    /** Get audio rendering parameters */
    AAudioParameters* GetAudioParameters() { return AudioParameters; }

    /** Get input mappings of input component */
    AInputMappings* GetInputMappings();

    /** Return input component */
    AInputComponent* GetInputComponent() const { return InputComponent; }

    /** Get cursor location in viewport-relative coordinates */
    Float2 GetLocalCursorPosition() const;

    /** Get normalized cursor location in viewport-relative coordinates */
    Float2 GetNormalizedCursorPosition() const;

    /** Get controller player index */
    int GetPlayerIndex() const;

    /** Primary audio listener */
    static APlayerController* GetCurrentAudioListener();

    virtual void OnViewportUpdate();

    virtual void UpdatePawnCamera();

protected:
    AInputComponent* InputComponent;

    APlayerController();
    ~APlayerController();

    void Initialize(SActorInitializer& Initializer);

    void OnPawnChanged() override;

private:
    void TogglePause();
    void TakeScreenshot();

    TRef<ARenderingParameters> RenderingParameters;
    TRef<AAudioParameters>     AudioParameters;
    TWeakRef<ASceneComponent>  AudioListener;
    TWeakRef<AHUD>             HUD;
    float                      ViewportAspectRatio{};
    int                        ViewportWidth{};
    int                        ViewportHeight{};

    static APlayerController* CurrentAudioListener;
};
