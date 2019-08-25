/*

Angie Engine Source Code

MIT License

Copyright (C) 2017-2019 Alexander Samusev.

This file is part of the Angie Engine Source Code.

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

#include "SpatialObject.h"

enum { MAX_SHADOW_CASCADES = 4 };

#define MIN_LIGHT_RADIUS 0.001f
#define MIN_LIGHT_TEMPERATURE 1000.0f
#define MAX_LIGHT_TEMPERATURE 40000.0f

enum ELightType {
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_SPOT,

    // FUTURE:
    //LIGHT_TYPE_TUBE
    //LIGHT_TYPE_AREA
};

enum ELightShadowTechnique {
    LIGHT_SHADOW_DISABLED,
    LIGHT_SHADOW_DEFAULT,

    // FUTURE?:
    //LIGHT_SHADOW_HARD,
    //LIGHT_SHADOW_SMOOTH
};

/*

FClusteredObject

*/
class FClusteredObject : public FSceneComponent {
    AN_COMPONENT( FClusteredObject, FSceneComponent )

    friend class FLevel;

public:
    enum { RENDERING_GROUP_DEFAULT = 1 };

    // Rendering group to filter lights during rendering
    int RenderingGroup;

    void ForceOutdoor( bool _OutdoorSurface );

    bool IsOutdoor() const { return bIsOutdoor; }

    BvSphereSSE const & GetSphereWorldBounds() const;
    BvAxisAlignedBox const & GetAABBWorldBounds() const;
    BvOrientedBox const & GetOBBWorldBounds() const;
    Float4x4 const & GetOBBTransformInverse() const;

    // TODO: SetVisible, IsVisible And only add visible objects to areas!!!

    void MarkAreaDirty();

    static void _UpdateSurfaceAreas();

protected:

    FClusteredObject();

    void InitializeComponent() override;
    void DeinitializeComponent() override;
    //void OnTransformDirty() override;

    BvSphere SphereWorldBounds;
    BvAxisAlignedBox AABBWorldBounds;
    BvOrientedBox OBBWorldBounds;
    Float4x4 OBBTransformInverse;

    FAreaLinks InArea; // list of intersected areas
    bool bIsOutdoor;

    FClusteredObject * NextDirty;
    FClusteredObject * PrevDirty;

    static FClusteredObject * DirtyList;
    static FClusteredObject * DirtyListTail;
};

class ANGIE_API FLightComponent : public FClusteredObject {
    AN_COMPONENT( FLightComponent, FClusteredObject )

public:
    void SetType( ELightType _Type );
    ELightType GetType() const;

    void SetShadowTechnique( ELightShadowTechnique _ShadowTechnique );
    ELightShadowTechnique GetShadowTechnique() const;

    void SetColor( Float3 const & _Color );
    void SetColor( float _R, float _G, float _B );
    Float3 const & GetColor() const;

    Float4 const & GetEffectiveColor() const;

    // Set temperature of the light in Kelvin
    void SetTemperature( float _Temperature );
    float GetTemperature() const;

    void SetLumens( float _Lumens );
    float GetLumens() const;

    void SetAmbientIntensity( float _Intensity );
    float GetAmbientIntensity() const;

    void SetInnerRadius( float _Radius );
    float GetInnerRadius() const;

    void SetOuterRadius( float _Radius );
    float GetOuterRadius() const;

    void SetInnerConeAngle( float _Angle );
    float GetInnerConeAngle() const;

    void SetOuterConeAngle( float _Angle );
    float GetOuterConeAngle() const;

    void SetDirection( Float3 const & _Direction );
    Float3 GetDirection() const;

    void SetWorldDirection( Float3 const & _Direction );
    Float3 GetWorldDirection() const;

    void SetSpotExponent( float _Exponent );
    float GetSpotExponent() const;    

    void SetMaxShadowCascades( int _MaxShadowCascades );
    int GetMaxShadowCascades() const;

    Float4x4 const & GetShadowMatrix() const;
    Float4x4 const & GetLightViewMatrix() const;

    static constexpr float MinLightRadius() { return MIN_LIGHT_RADIUS; }
    static constexpr float MinLightTemperature() { return MIN_LIGHT_TEMPERATURE; }
    static constexpr float MaxLightTemperature() { return MAX_LIGHT_TEMPERATURE; }

protected:
    FLightComponent();

    void DrawDebug( FDebugDraw * _DebugDraw ) override;
    void OnTransformDirty() override;

private:
    void UpdateBoundingBox();

    ELightType              Type;
    ELightShadowTechnique   ShadowTechnique;
    Float3                  Color;
    mutable Float4          EffectiveColor;         // Composed from Temperature, Lumens, Color and AmbientIntensity
    mutable bool            bEffectiveColorDirty;
    float                   Temperature;
    float                   Lumens;
    float                   InnerRadius;
    float                   OuterRadius;
    float                   InnerConeAngle;
    float                   OuterConeAngle;
    float                   SpotExponent;
    int                     MaxShadowCascades;
    mutable Float4x4        LightViewMatrix;
    mutable Float4x4        ShadowMatrix;
    mutable bool            bShadowMatrixDirty;
};
