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

#include <Engine/Core/Public/BV/BvFrustum.h>

enum ECameraProjection : uint8_t {
    CAMERA_PROJ_ORTHOGRAPHIC,
    CAMERA_PROJ_PERSPECTIVE,
};

enum ECameraPerspectiveAdjust : uint8_t {
    CAMERA_ADJUST_FOV_X_ASPECT_RATIO,
    CAMERA_ADJUST_FOV_X_FOV_Y,
};

class ANGIE_API ACameraComponent : public ASceneComponent {
    AN_COMPONENT( ACameraComponent, ASceneComponent )

public:
    void SetProjection( ECameraProjection _Projection );

    void SetPerspective() { SetProjection( CAMERA_PROJ_PERSPECTIVE ); }

    void SetOrthographic() { SetProjection( CAMERA_PROJ_ORTHOGRAPHIC ); }

    void SetPerspectiveAdjust( ECameraPerspectiveAdjust _Adjust );

    void SetZNear( float _ZNear );

    void SetZFar( float _ZFar );

    void SetFovX( float _FieldOfView );

    void SetFovY( float _FieldOfView );

    /** Perspective aspect ratio. For example 4/3, 16/9 */
    void SetAspectRatio( float _AspectRatio );

    /** Use aspect ratio from monitor geometry. Use it for fullscreen video mode. */
    void SetMonitorAspectRatio( struct SPhysicalMonitor const * _Monitor );

    // Use aspect ratio from window geometry. Use it for windowed mode.
    // Call this on each window resize to update aspect ratio.
    //void SetWindowAspectRatio( FWindow const * _Window );

    void SetOrthoRect( Float2 const & _Mins, Float2 const & _Maxs );

    ECameraProjection GetProjection() const { return Projection; }

    bool IsPerspective() const { return Projection == CAMERA_PROJ_PERSPECTIVE; }

    bool IsOrthographic() const { return Projection == CAMERA_PROJ_ORTHOGRAPHIC; }

    ECameraPerspectiveAdjust GetPerspectiveAdjust() const { return Adjust; }

    float GetZNear() const { return ZNear; }

    float GetZFar() const { return ZFar; }

    float GetFovX() const { return FovX; }

    float GetFovY() const { return FovY; }

    float GetAspectRatio() const { return AspectRatio; }

    /** Computes real camera field of view in radians */
    void GetEffectiveFov( float & _FovX, float & _FovY ) const;

    Float2 const & GetOrthoMins() const { return OrthoMins; }

    Float2 const & GetOrthoMaxs() const { return OrthoMaxs; }

    Float4x4 const & GetProjectionMatrix() const;

    Float4x4 const & GetViewMatrix() const;

    Float3x3 const & GetBillboardMatrix() const;

    BvFrustum const & GetFrustum() const;

    /** NormalizedX = ScreenX / ScreenWidth, NormalizedY = ScreenY / ScreenHeight */
    void MakeRay( float _NormalizedX, float _NormalizedY, Float3 & _RayStart, Float3 & _RayEnd ) const;

    static void MakeRay( Float4x4 const & _ModelViewProjectionInversed, float _NormalizedX, float _NormalizedY, Float3 & _RayStart, Float3 & _RayEnd );

    /** Compute ortho rect based on aspect ratio and zoom */
    static void MakeOrthoRect( float _CameraAspectRatio, float _Zoom, Float2 & _Mins, Float2 & _Maxs );

    void MakeClusterProjectionMatrix( Float4x4 & _ProjectionMatrix/*, const float _ClusterZNear, const float _ClusterZFar*/ ) const;

protected:
    ACameraComponent();

    void OnTransformDirty() override;

    void DrawDebug( ADebugDraw * _DebugDraw ) override;

private:
    float               FovX;
    float               FovY;
    float               ZNear;
    float               ZFar;
    float               AspectRatio;
    Float2              OrthoMins;
    Float2              OrthoMaxs;    
    mutable Float4x4    ViewMatrix;
    mutable Float3x3    BillboardMatrix;    
    mutable Float4x4    ProjectionMatrix;    
    mutable BvFrustum   Frustum;
    ECameraProjection   Projection;
    ECameraPerspectiveAdjust Adjust;
    mutable bool        bViewMatrixDirty;
    mutable bool        bProjectionDirty;
    mutable bool        bFrustumDirty;
};
