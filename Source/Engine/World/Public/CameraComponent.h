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

#include "SceneComponent.h"

#include <Engine/Core/Public/BV/Frustum.h>

class ANGIE_API FCameraComponent : public FSceneComponent {
    AN_COMPONENT( FCameraComponent, FSceneComponent )

public:
    enum EProjectionType {
        ORTHOGRAPHIC,
        PERSPECTIVE
    };

    enum EAdjustPerspective {
        ADJ_FOV_X_ASPECT_RATIO,
        ADJ_FOV_X_FOV_Y
    };


    //void                DrawDebug( class FCameraComponent * _Camera, EDebugDrawFlags::Type _DebugDrawFlags, class FPrimitiveBatchComponent * _DebugDraw );

    void                SetProjection( int _Projection );
    int                 GetProjection() const { return Projection; }

    void                SetPerspective() { SetProjection( PERSPECTIVE ); }
    void                SetOrthographic() { SetProjection( ORTHOGRAPHIC ); }

    bool                IsPerspective() const { return Projection == PERSPECTIVE; }
    bool                IsOrthographic() const { return Projection == ORTHOGRAPHIC; }

    void                SetPerspectiveAdjust( int _Adjust );
    int                 GetPerspectiveAdjust() const { return Adjust; }

    void                SetZNear( float _ZNear );
    float               GetZNear() const { return ZNear; }

    void                SetZFar( float _ZFar );
    float               GetZFar() const { return ZFar; }

    void                SetFovX( float _FieldOfView );
    float               GetFovX() const { return FovX; }

    void                SetFovY( float _FieldOfView );
    float               GetFovY() const { return FovY; }

    // Perspective aspect ratio. For example 4/3, 16/9
    void                SetAspectRatio( float _AspectRatio );

    // Use aspect ratio from monitor geometry. Use it for fullscreen video mode.
    void                SetMonitorAspectRatio( const struct FPhysicalMonitor * _Monitor );

    // Use aspect ratio from window geometry. Use it for windowed mode.
    // Call this on each window resize to update aspect ratio.
    //void                SetWindowAspectRatio( const class FVirtualDisplay * _Window );

    float               GetAspectRatio() const { return AspectRatio; }

    // Computes real camera field of view in radians
    void                GetEffectiveFov( float & _FovX, float & _FovY ) const;

    void                SetOrthoRect( const Float4 & _OrthoRect );
    Float4 const &      GetOrthoRect() const { return OrthoRect; }

    void                SetOrthoRect( float _Left, float _Right, float _Bottom, float _Top );

    // Compute ortho rect. Based on aspect ratio
    void                ComputeRect( float _OrthoZoom, float * _Left, float * _Right, float * _Bottom, float * _Top ) const;

    // NormalizedX = ScreenX / ScreenWidth, NormalizedY = ScreenY / ScreenHeight
    SegmentF            GetRay( float _NormalizedX, float _NormalizedY ) const;

    Float4x4 const &    GetProjectionMatrix() const;
    Float4x4 const &    GetViewMatrix() const;
    Float3x3 const &    GetBillboardMatrix() const;
    FFrustum const &    GetFrustum() const;

    void                ComputeClusterProjectionMatrix( Float4x4 & _Matrix, const float _ClusterZNear, const float _ClusterZFar ) const;

protected:
    FCameraComponent();

    void InitializeComponent() override;
    void BeginPlay() override;
    void EndPlay() override;
    void TickComponent( float _TimeStep ) override;
    void OnTransformDirty() override;

private:
    int                 Projection;
    float               FovX;
    float               FovY;
    float               ZNear;
    float               ZFar;
    float               AspectRatio;
    Float4              OrthoRect;
    int                 Adjust;
    mutable Float4x4    ViewMatrix;
    mutable Float3x3    BillboardMatrix;
    mutable bool        ViewMatrixDirty;
    mutable Float4x4    ProjectionMatrix;
    mutable bool        ProjectionDirty;
    mutable FFrustum    Frustum;
    mutable bool        FrustumDirty;
};
