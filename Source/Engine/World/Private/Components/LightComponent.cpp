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

#include <Engine/World/Public/Components/LightComponent.h>
#include <Engine/World/Public/DebugDraw.h>

#define DEFAULT_AMBIENT_INTENSITY 1.0f
#define DEFAULT_TEMPERATURE 6590.0f
#define DEFAULT_INNER_RADIUS 0.5f
#define DEFAULT_OUTER_RADIUS 1.0f
#define DEFAULT_INNER_CONE_ANGLE 30.0f
#define DEFAULT_OUTER_CONE_ANGLE 35.0f
#define DEFAULT_LUMENS 3000.0f
#define DEFAULT_COLOR Float3(1)
#define DEFAULT_SPOT_EXPONENT 1.0f
#define DEFAULT_MAX_SHADOW_CASCADES 4

AN_CLASS_META( FLightComponent )
//AN_SCENE_COMPONENT_BEGIN_DECL( FLightComponent, CCF_DEFAULT )
//AN_ATTRIBUTE( "Type", FProperty( (int)FLightComponent::T_Point ), SetType, GetType, "Directional\0Point\0Spot\0\0Light type", AF_ENUM )
//AN_ATTRIBUTE( "Shadow Technique", FProperty( (int)FLightComponent::DefaultShadows ), SetShadowTechnique, GetShadowTechnique, "No Shadows\0Default Shadows\0\0Light shadow casting", AF_ENUM )
//AN_ATTRIBUTE( "Color", FProperty( DEFAULT_COLOR ), SetColor, GetColor, "Light color", AF_COLOR )
//AN_ATTRIBUTE( "Temperature", FProperty( DEFAULT_TEMPERATURE ), SetTemperature, GetTemperature, "Light temperature", AF_DEFAULT )
//AN_ATTRIBUTE( "Lumens", FProperty( DEFAULT_LUMENS ), SetLumens, GetLumens, "Light lumens", AF_DEFAULT )
//AN_ATTRIBUTE( "Ambient Intensity", FProperty( DEFAULT_AMBIENT_INTENSITY ), SetAmbientIntensity, GetAmbientIntensity, "Light ambient intensity", AF_DEFAULT )
//AN_ATTRIBUTE( "Inner Radius", FProperty( DEFAULT_INNER_RADIUS ), SetInnerRadius, GetInnerRadius, "Light inner radius", AF_DEFAULT )
//AN_ATTRIBUTE( "Outer Radius", FProperty( DEFAULT_OUTER_RADIUS ), SetOuterRadius, GetOuterRadius, "Light outer radius", AF_DEFAULT )
//AN_ATTRIBUTE( "Inner Cone Angle", FProperty( DEFAULT_INNER_CONE_ANGLE ), SetInnerConeAngle, GetInnerConeAngle, "Spot light inner cone angle", AF_DEFAULT )
//AN_ATTRIBUTE( "Outer Cone Angle", FProperty( DEFAULT_OUTER_CONE_ANGLE ), SetOuterConeAngle, GetOuterConeAngle, "Spot light outer cone angle", AF_DEFAULT )
//AN_ATTRIBUTE( "Spot Exponent", FProperty( DEFAULT_SPOT_EXPONENT ), SetSpotExponent, GetSpotExponent, "Spot light exponent", AF_DEFAULT )
//AN_ATTRIBUTE( "Max Shadow Cascades", FProperty( DEFAULT_MAX_SHADOW_CASCADES ), SetMaxShadowCascades, GetMaxShadowCascades, "Max dynamic shadow cascades", AF_DEFAULT )
//AN_ATTRIBUTE( "Render Mask", FProperty( RENDERING_GROUP_DEFAULT ), SetRenderMask, GetRenderMask, "Set render mask", AF_BITFIELD )
//AN_SCENE_COMPONENT_END_DECL

FLightComponent::FLightComponent() {
    Type = LIGHT_TYPE_POINT;
    ShadowTechnique = LIGHT_SHADOW_DEFAULT;
    Color = DEFAULT_COLOR;
    Temperature = DEFAULT_TEMPERATURE;
    Lumens = DEFAULT_LUMENS;
    EffectiveColor = Float4(0.0f,0.0f,0.0f,DEFAULT_AMBIENT_INTENSITY);
    bEffectiveColorDirty = true;
    InnerRadius = DEFAULT_INNER_RADIUS;
    OuterRadius = DEFAULT_OUTER_RADIUS;
    InnerConeAngle = DEFAULT_INNER_CONE_ANGLE;
    OuterConeAngle = DEFAULT_OUTER_CONE_ANGLE;
    SpotExponent = DEFAULT_SPOT_EXPONENT;
    MaxShadowCascades = DEFAULT_MAX_SHADOW_CASCADES;
    bShadowMatrixDirty = true;
    RenderingGroup = RENDERING_GROUP_DEFAULT;
#ifdef FUTURE
    SphereWorldBounds.Radius = OuterRadius;
    SphereWorldBounds.Center = Float3(0);
    AABBWorldBounds.Mins = SphereWorldBounds.Center - OuterRadius;
    AABBWorldBounds.Maxs = SphereWorldBounds.Center + OuterRadius;
#endif

    UpdateBoundingBox();
}

void FLightComponent::SetType( ELightType _Type ) {
    if ( Type == _Type ) {
        return;
    }

    Type = _Type;

    UpdateBoundingBox();
}

ELightType FLightComponent::GetType() const {
    return Type;
}

void FLightComponent::SetShadowTechnique( ELightShadowTechnique _ShadowTechnique ) {
    ShadowTechnique = _ShadowTechnique;
}

ELightShadowTechnique FLightComponent::GetShadowTechnique() const {
    return ShadowTechnique;
}

void FLightComponent::SetInnerRadius( float _Radius ) {
    InnerRadius = FMath::Max( MIN_LIGHT_RADIUS, _Radius );
}

float FLightComponent::GetInnerRadius() const {
    return InnerRadius;
}

void FLightComponent::SetOuterRadius( float _Radius ) {
    OuterRadius = FMath::Max( MIN_LIGHT_RADIUS, _Radius );

    UpdateBoundingBox();
}

float FLightComponent::GetOuterRadius() const {
    return OuterRadius;
}

void FLightComponent::SetInnerConeAngle( float _Angle ) {
    InnerConeAngle = FMath::Clamp( _Angle, 0.0001f, 180.0f );
}

float FLightComponent::GetInnerConeAngle() const {
    return InnerConeAngle;
}

void FLightComponent::SetOuterConeAngle( float _Angle ) {
    OuterConeAngle = FMath::Clamp( _Angle, 0.0001f, 180.0f );
    if ( Type == LIGHT_TYPE_SPOT ) {
        UpdateBoundingBox();
    }
}

float FLightComponent::GetOuterConeAngle() const {
    return OuterConeAngle;
}

void FLightComponent::SetSpotExponent( float _Exponent ) {
    SpotExponent = _Exponent;
}

float FLightComponent::GetSpotExponent() const {
    return SpotExponent;
}

void FLightComponent::SetDirection( Float3 const & _Direction ) {
    Float3x3 orientation;

    orientation[2] = -_Direction.Normalized();
    orientation[2].ComputeBasis( orientation[0], orientation[1] );

    Quat rotation;
    rotation.FromMatrix( orientation );
    SetRotation( rotation );

    bShadowMatrixDirty = true;
}

Float3 FLightComponent::GetDirection() const {
    return GetForwardVector();
}

void FLightComponent::SetWorldDirection( Float3 const & _Direction ) {
    Float3x3 orientation;
    orientation[2] = -_Direction.Normalized();
    orientation[2].ComputeBasis( orientation[0], orientation[1] );

    Quat rotation;
    rotation.FromMatrix( orientation );
    SetWorldRotation( rotation );

    bShadowMatrixDirty = true;
}

Float3 FLightComponent::GetWorldDirection() const {
    return GetWorldForwardVector();
}

void FLightComponent::SetColor( Float3 const & _Color ) {
    Color = _Color;
    bEffectiveColorDirty = true;
}

void FLightComponent::SetColor( float _R, float _G, float _B ) {
    Color.X = _R;
    Color.Y = _G;
    Color.Z = _B;
    bEffectiveColorDirty = true;
}

Float3 const & FLightComponent::GetColor() const {
    return Color;
}

// Convert temperature in Kelvins to RGB color. Assume temperature is range between 1000 and 40000.
// Based on code by Benjamin 'BeRo' Rosseaux
static Float3 GetRGBFromTemperature( float _Temperature ) {
    Float3 Result;

    if ( _Temperature <= 6500.0f ) {
        Result.X = 1.0f;
        Result.Y = -2902.1955373783176f / ( 1669.5803561666639f + _Temperature ) + 1.3302673723350029f;
        Result.Z = -8257.7997278925690f / ( 2575.2827530017594f + _Temperature ) + 1.8993753891711275f;

        Result.Z = FMath::Max< float >( 0.0f, Result.Z );
    } else {
        Result.X = 1745.0425298314172f / ( -2666.3474220535695f + _Temperature ) + 0.55995389139931482f;
        Result.Y = 1216.6168361476490f / ( -2173.1012343082230f + _Temperature ) + 0.70381203140554553f;
        Result.Z = -8257.7997278925690f / ( 2575.2827530017594f + _Temperature ) + 1.8993753891711275f;

        Result.X = FMath::Min< float >( 1.0f, Result.X );
        Result.Z = FMath::Min< float >( 1.0f, Result.Z );
    }

    return Result;
}

// Convert temperature in Kelvins to RGB color. Assume temperature is range between 1000 and 40000.
// Based on code from http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
static Float3 GetRGBFromTemperature2( float _Temperature ) {
    Float3 Result;
    float Value;

    // All calculations require _Temperature / 100, so only do the conversion once
    _Temperature *= 0.01f;

    // Calculate each color in turn

    if ( _Temperature <= 66 ) {
        Result.X = 1.0f;

        // Note: the R-squared value for this approximation is .996
        Value = (99.4708025861/255.0) * log(_Temperature) - (161.1195681661/255.0);
        Result.Y = FMath::Min( 1.0f, Value );//FMath::Clamp( Value, 0.0f, 1.0f );

    } else {
        // Note: the R-squared value for this approximation is .988
        Value = ( 329.698727446 / 255.0 ) * pow(_Temperature - 60, -0.1332047592);
        Result.X = FMath::Min( 1.0f, Value );//FMath::Clamp( Value, 0.0f, 1.0f );

        // Note: the R-squared value for this approximation is .987
        Value = (288.1221695283/255.0) * pow(_Temperature - 60, -0.0755148492);
        Result.Y = Value;//FMath::Clamp( Value, 0.0f, 1.0f );
    }

    if ( _Temperature >= 66 ) {
        Result.Z = 1.0f;
    } else if ( _Temperature <= 19 ) {
        Result.Z = 0.0f;
    } else {
        // Note: the R-squared value for this approximation is .998
        Value = (138.5177312231/255.0) * log(_Temperature - 10) - (305.0447927307/255.0);
        Result.Z = FMath::Max( 0.0f, Value );//FMath::Clamp( Value, 0.0f, 1.0f );
    }

    return Result;
}

#if 0
// Convert temperature in Kelvins to RGB color. Assume temperature is range between 1000 and 10000.
// Based on Urho sources
static Float3 GetRGBFromTemperature( float _Temperature ) {
    // Approximate Planckian locus in CIE 1960 UCS
    float u = (0.860117757f + 1.54118254e-4f * _Temperature + 1.28641212e-7f * _Temperature * _Temperature) /
        (1.0f + 8.42420235e-4f * _Temperature + 7.08145163e-7f * _Temperature * _Temperature);
    float v = (0.317398726f + 4.22806245e-5f * _Temperature + 4.20481691e-8f * _Temperature * _Temperature) /
        (1.0f - 2.89741816e-5f * _Temperature + 1.61456053e-7f * _Temperature * _Temperature);

    float x = 3.0f * u / (2.0f * u - 8.0f * v + 4.0f);
    float y = 2.0f * v / (2.0f * u - 8.0f * v + 4.0f);
    float z = 1.0f - x - y;

    float y_ = 1.0f;
    float x_ = y_ / y * x;
    float z_ = y_ / y * z;

    Float3 Color;
    Color.X = 3.2404542f * x_ + -1.5371385f * y_ + -0.4985314f * z_;
    Color.Y = -0.9692660f * x_ + 1.8760108f * y_ + 0.0415560f * z_;
    Color.Z = 0.0556434f * x_ + -0.2040259f * y_ + 1.0572252f * z_;

    return Color;
}
#endif

static const float LumensToEnergy = 4.0f * 16.0f / 10000.0f;

static float GetLightEnergyFromLumens( float _Lumens ) {
    return _Lumens * LumensToEnergy;
}

Float4 const & FLightComponent::GetEffectiveColor() const {
    if ( bEffectiveColorDirty ) {
        *(Float3 *)&EffectiveColor = Color * GetRGBFromTemperature( Temperature ) * GetLightEnergyFromLumens( Lumens );
        bEffectiveColorDirty = false;
    }
    return EffectiveColor;
}

void FLightComponent::SetTemperature( float _Temperature ) {
    Temperature = FMath::Clamp( _Temperature, MIN_LIGHT_TEMPERATURE, MAX_LIGHT_TEMPERATURE );
    bEffectiveColorDirty = true;
}

float FLightComponent::GetTemperature() const {
    return Temperature;
}

void FLightComponent::SetLumens( float _Lumens ) {
    Lumens = FMath::Max( 0.0f, _Lumens );
    bEffectiveColorDirty = true;
}

float FLightComponent::GetLumens() const {
    return Lumens;
}

void FLightComponent::SetAmbientIntensity( float _Intensity ) {
    EffectiveColor.W = FMath::Max( 0.1f, _Intensity );
}

float FLightComponent::GetAmbientIntensity() const {
    return EffectiveColor.W;
}

void FLightComponent::SetMaxShadowCascades( int _MaxShadowCascades ) {
    MaxShadowCascades = Int( _MaxShadowCascades ).Clamp( 1, MAX_SHADOW_CASCADES );
}

int FLightComponent::GetMaxShadowCascades() const {
    return MaxShadowCascades;
}

void FLightComponent::OnTransformDirty() {
    Super::OnTransformDirty();

    bShadowMatrixDirty = true;

    UpdateBoundingBox();
    MarkAreaDirty();
}

void FLightComponent::UpdateBoundingBox() {
    switch ( Type ) {
        case LIGHT_TYPE_DIRECTIONAL:
        {
            SphereWorldBounds.Radius = CONVEX_HULL_MAX_BOUNDS * 1.4f;
            SphereWorldBounds.Center = Float3( 0.0f );

            AABBWorldBounds.Mins = Float3( CONVEX_HULL_MIN_BOUNDS );
            AABBWorldBounds.Maxs = Float3( CONVEX_HULL_MAX_BOUNDS );

            OBBWorldBounds.Center = SphereWorldBounds.Center;
            OBBWorldBounds.HalfSize = Float3( CONVEX_HULL_MAX_BOUNDS );
            OBBWorldBounds.Orient.SetIdentity();
            break;
        }
        case LIGHT_TYPE_POINT:
        {
            SphereWorldBounds.Radius = OuterRadius;
            SphereWorldBounds.Center = GetWorldPosition();

            AABBWorldBounds.Mins = SphereWorldBounds.Center - OuterRadius;
            AABBWorldBounds.Maxs = SphereWorldBounds.Center + OuterRadius;

            OBBWorldBounds.Center = SphereWorldBounds.Center;
            OBBWorldBounds.HalfSize = Float3( SphereWorldBounds.Radius );
            OBBWorldBounds.Orient.SetIdentity();

            // TODO: Optimize?
            Float4x4 OBBTransform = Float4x4::Translation( OBBWorldBounds.Center ) * Float4x4::Scale( OBBWorldBounds.HalfSize );
            OBBTransformInverse = OBBTransform.Inversed();
            break;
        }
        case LIGHT_TYPE_SPOT:
        {
            const float ToHalfAngleRadians = 0.5f / 180.0f * FMath::_PI;
            const float HalfConeAngle = OuterConeAngle * ToHalfAngleRadians;
            const Float3 WorldPos = GetWorldPosition();

            // Compute cone OBB for voxelization
            OBBWorldBounds.Orient = GetWorldRotation().ToMatrix();

            const Float3 SpotDir = -OBBWorldBounds.Orient[ 2 ];

            //OBBWorldBounds.HalfSize.X = OBBWorldBounds.HalfSize.Y = tan( HalfConeAngle ) * OuterRadius;
            OBBWorldBounds.HalfSize.X = OBBWorldBounds.HalfSize.Y = sin( HalfConeAngle ) * OuterRadius;
            OBBWorldBounds.HalfSize.Z = OuterRadius * 0.5f;
            OBBWorldBounds.Center = WorldPos + SpotDir * ( OBBWorldBounds.HalfSize.Z );

            // TODO: Optimize?
            Float4x4 OBBTransform = Float4x4::Translation( OBBWorldBounds.Center ) * Float4x4( OBBWorldBounds.Orient ) * Float4x4::Scale( OBBWorldBounds.HalfSize );
            OBBTransformInverse = OBBTransform.Inversed();

            // Compute cone AABB for culling
            AABBWorldBounds.Clear();
            AABBWorldBounds.AddPoint( WorldPos );
            Float3 v = WorldPos + SpotDir * OuterRadius;
            Float3 vx = OBBWorldBounds.Orient[ 0 ] * OBBWorldBounds.HalfSize.X;
            Float3 vy = OBBWorldBounds.Orient[ 1 ] * OBBWorldBounds.HalfSize.X;
            AABBWorldBounds.AddPoint( v + vx );
            AABBWorldBounds.AddPoint( v - vx );
            AABBWorldBounds.AddPoint( v + vy );
            AABBWorldBounds.AddPoint( v - vy );

            // Посмотреть, как более эффективно распределяется площадь - у сферы или AABB
            // Compute cone Sphere bounds
            if ( HalfConeAngle > FMath::_PI / 4 ) {
                SphereWorldBounds.Radius = sin( HalfConeAngle ) * OuterRadius;
                SphereWorldBounds.Center = WorldPos + SpotDir * ( cos( HalfConeAngle ) * OuterRadius );
            } else {
                SphereWorldBounds.Radius = OuterRadius / ( 2.0 * cos( HalfConeAngle ) );
                SphereWorldBounds.Center = WorldPos + SpotDir * SphereWorldBounds.Radius;
            }
            break;
        }
    }
}

Float4x4 const & FLightComponent::GetShadowMatrix() const {
    if ( bShadowMatrixDirty ) {
        // Update shadow matrix

        #define DEFAULT_ZNEAR 0.04f//0.1f
        #define DEFAULT_ZFAR 10000.0f//800.0f//10000.0f//99999.0f

        Float3x3 BillboardMatrix = GetWorldRotation().ToMatrix();
        Float3x3 Basis = BillboardMatrix.Transposed();
        //Float3 Origin = -Basis[2] * 1000.0f;//Basis * ( -GetWorldPosition() );
        Float3 Origin = Basis * ( -GetWorldBackVector()*400.0f );

        LightViewMatrix[ 0 ] = Float4( Basis[ 0 ], 0.0f );
        LightViewMatrix[ 1 ] = Float4( Basis[ 1 ], 0.0f );
        LightViewMatrix[ 2 ] = Float4( Basis[ 2 ], 0.0f );
        LightViewMatrix[ 3 ] = Float4( Origin, 1.0f );

        //const float SHADOWMAP_SIZE = 4096;
        //Float4 OrthoRect = Float4( -1, 1, -1, 1 )*150.0f;//(SHADOWMAP_SIZE*0.5f/100.0f);
        Float2 OrthoMins = Float2( -1,-1 )*150.0f;
        Float2 OrthoMaxs = Float2(  1, 1 )*150.0f;
        Float4x4 ProjectionMatrix = Float4x4::OrthoCC( OrthoMins, OrthoMaxs, DEFAULT_ZNEAR, DEFAULT_ZFAR );
        //Float4x4 ProjectionMatrix = FMath::PerspectiveProjectionMatrixCC( FMath::_PI/1.1f,FMath::_PI/1.1f,DEFAULT_ZNEAR, DEFAULT_ZFAR );


        //Float4x4 Test = FMath::PerspectiveProjectionMatrix( OrthoRect.X, OrthoRect.Y, OrthoRect.Z, OrthoRect.W, DEFAULT_ZNEAR, DEFAULT_ZFAR );
        //Float4x4 Test = PerspectiveProjectionMatrixRevCC( FMath::_PI/4,FMath::_PI/4,DEFAULT_ZNEAR, DEFAULT_ZFAR );




        //Float4 v1 = Test * Float4(0,0,-DEFAULT_ZFAR,1.0f);
        //v1/=v1.W;
        //Float4 v2 = Test * Float4(0,0,-DEFAULT_ZNEAR,1.0f);
        //v2/=v2.W;

        //Float4x4 ProjectionMatrix = PerspectiveProjectionMatrixRevCC( FMath::_PI/4.0f, FMath::_PI/4.0f, /*SHADOWMAP_SIZE, SHADOWMAP_SIZE, */DEFAULT_ZNEAR, DEFAULT_ZFAR );
        ShadowMatrix = ProjectionMatrix * LightViewMatrix;

        bShadowMatrixDirty = false;
    }

    return ShadowMatrix;
}

const Float4x4 & FLightComponent::GetLightViewMatrix() const {
    GetShadowMatrix(); // Update matrix

    return LightViewMatrix;
}

void FLightComponent::DrawDebug( FDebugDraw * _DebugDraw ) {

    Super::DrawDebug( _DebugDraw );

    if ( RVDrawLights ) {
        Float3 pos = GetWorldPosition();
        Float3x3 orient = GetWorldRotation().ToMatrix();

        switch ( Type ) {
        case LIGHT_TYPE_DIRECTIONAL:
            _DebugDraw->SetDepthTest( false );
            _DebugDraw->SetColor( FColor4( 1, 1, 1, 1 ) );
            _DebugDraw->DrawLine( pos, pos + GetWorldDirection() * 10.0f );
            break;
        case LIGHT_TYPE_POINT:
            _DebugDraw->SetDepthTest( false );
            _DebugDraw->SetColor( FColor4( 0.5f, 0.5f, 0.5f, 1 ) );
            _DebugDraw->DrawCone( pos, orient, InnerRadius, InnerConeAngle * 0.5f );
            _DebugDraw->SetColor( FColor4( 1, 1, 1, 1 ) );
            _DebugDraw->DrawCone( pos, orient, OuterRadius, OuterConeAngle * 0.5f );
            break;
        case LIGHT_TYPE_SPOT:
            _DebugDraw->SetDepthTest( false );
            _DebugDraw->SetColor( FColor4( 0.5f, 0.5f, 0.5f, 1 ) );
            _DebugDraw->DrawCone( pos, orient, InnerRadius, InnerConeAngle * 0.5f );
            _DebugDraw->SetColor( FColor4( 1, 1, 1, 1 ) );
            _DebugDraw->DrawCone( pos, orient, OuterRadius, OuterConeAngle * 0.5f );
            break;
        }
    }
}










#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_BEGIN_CLASS_META( FClusteredObject )
AN_END_CLASS_META()

FClusteredObject * FClusteredObject::DirtyList = nullptr;
FClusteredObject * FClusteredObject::DirtyListTail = nullptr;

FClusteredObject::FClusteredObject() {
    RenderingGroup = RENDERING_GROUP_DEFAULT;
}

BvSphereSSE const & FClusteredObject::GetSphereWorldBounds() const {
    return SphereWorldBounds;
}

BvAxisAlignedBox const & FClusteredObject::GetAABBWorldBounds() const {
    return AABBWorldBounds;
}

BvOrientedBox const & FClusteredObject::GetOBBWorldBounds() const {
    return OBBWorldBounds;
}

Float4x4 const & FClusteredObject::GetOBBTransformInverse() const {
    return OBBTransformInverse;
}

void FClusteredObject::InitializeComponent() {
    Super::InitializeComponent();

    MarkAreaDirty();
}

void FClusteredObject::DeinitializeComponent() {
    Super::DeinitializeComponent();
#if 0
    // remove from dirty list
    IntrusiveRemoveFromList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail );

    // FIXME: Is it right way to remove surface areas here?
    FWorld * world = GetWorld();
    for ( FLevel * level : world->GetArrayOfLevels() ) {
        level->RemoveSurfaceAreas( this );
    }
#endif
}

void FClusteredObject::MarkAreaDirty() {
#if 0
    // add to dirty list
    if ( !IntrusiveIsInList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail ) ) {
        IntrusiveAddToList( this, NextDirty, PrevDirty, DirtyList, DirtyListTail );
    }
#endif
}

void FClusteredObject::ForceOutdoor( bool _OutdoorSurface ) {
    if ( bIsOutdoor == _OutdoorSurface ) {
        return;
    }

    bIsOutdoor = _OutdoorSurface;
    MarkAreaDirty();
}

void FClusteredObject::_UpdateSurfaceAreas() {
#if 0
    FClusteredObject * next;
    for ( FClusteredObject * surf = DirtyList; surf; surf = next ) {

        next = surf->NextDirty;

        FWorld * world = surf->GetWorld();

        for ( FLevel * level : world->GetArrayOfLevels() ) {
            level->RemoveSurfaceAreas( surf );
        }

        for ( FLevel * level : world->GetArrayOfLevels() ) {
            level->AddSurfaceAreas( surf );
            //GLogger.Printf( "Update actor %s\n", surf->GetParentActor()->GetNameConstChar() );
        }

        surf->PrevDirty = surf->NextDirty = nullptr;
    }
#endif
    DirtyList = DirtyListTail = nullptr;
}
