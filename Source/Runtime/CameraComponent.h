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

#include "SceneComponent.h"

#include <Geometry/BV/BvFrustum.h>

enum ECameraProjection : uint8_t
{
    CAMERA_PROJ_ORTHO_RECT,
    CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO,
    CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y,
    CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO,
    CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO
};

class ACameraComponent : public ASceneComponent
{
    HK_COMPONENT(ACameraComponent, ASceneComponent)

public:
    /** Set view projection */
    void SetProjection(ECameraProjection _Projection);

    /** Near clip distance */
    void SetZNear(float _ZNear);

    /** Far clip distance */
    void SetZFar(float _ZFar);

    /** Viewport aspect ratio. For example 4/3, 16/9 */
    void SetAspectRatio(float _AspectRatio);

    /** Horizontal FOV for perspective projection */
    void SetFovX(float _FieldOfView);

    /** Vertical FOV for perspective projection */
    void SetFovY(float _FieldOfView);

    /** Rectangle for orthogonal projection */
    void SetOrthoRect(Float2 const& _Mins, Float2 const& _Maxs);

    /** Zoom for orthogonal projection */
    void SetOrthoZoom(float _Zoom);

    ECameraProjection GetProjection() const { return m_Projection; }

    bool IsPerspective() const
    {
        return m_Projection == CAMERA_PROJ_PERSPECTIVE_FOV_X_ASPECT_RATIO || m_Projection == CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO || m_Projection == CAMERA_PROJ_PERSPECTIVE_FOV_X_FOV_Y;
    }

    bool IsOrthographic() const { return m_Projection == CAMERA_PROJ_ORTHO_RECT || m_Projection == CAMERA_PROJ_ORTHO_ZOOM_ASPECT_RATIO; }

    float GetZNear() const { return m_ZNear; }

    float GetZFar() const { return m_ZFar; }

    float GetAspectRatio() const { return m_AspectRatio; }

    float GetFovX() const { return m_FovX; }

    float GetFovY() const { return m_FovY; }

    /** Computes real camera field of view in radians for perspective projection */
    void GetEffectiveFov(float& _FovX, float& _FovY) const;

    Float2 const& GetOrthoMins() const { return m_OrthoMins; }

    Float2 const& GetOrthoMaxs() const { return m_OrthoMaxs; }

    Float4x4 const& GetProjectionMatrix() const;

    Float4x4 const& GetViewMatrix() const;

    Float3x3 const& GetBillboardMatrix() const;

    BvFrustum const& GetFrustum() const;

    /** NormalizedX = ScreenX / ScreenWidth, NormalizedY = ScreenY / ScreenHeight */
    void MakeRay(float _NormalizedX, float _NormalizedY, Float3& _RayStart, Float3& _RayEnd) const;

    static void MakeRay(Float4x4 const& _ModelViewProjectionInversed, float _NormalizedX, float _NormalizedY, Float3& _RayStart, Float3& _RayEnd);

    /** Compute ortho rect based on aspect ratio and zoom */
    static void MakeOrthoRect(float _CameraAspectRatio, float _Zoom, Float2& _Mins, Float2& _Maxs);

    void MakeClusterProjectionMatrix(Float4x4& _ProjectionMatrix /*, const float _ClusterZNear, const float _ClusterZFar*/) const;

protected:
    ACameraComponent();

    void OnCreateAvatar() override;

    void OnTransformDirty() override;

    void DrawDebug(ADebugRenderer* InRenderer) override;

private:
    float             m_FovX{90.0f};
    float             m_FovY{90.0f};
    float             m_ZNear{0.04f};
    float             m_ZFar{99999.0f};
    float             m_AspectRatio{1.0f};
    Float2            m_OrthoMins{-1, -1};
    Float2            m_OrthoMaxs{1, 1};
    float             m_OrthoZoom{30};
    mutable Float4x4  m_ViewMatrix;
    mutable Float3x3  m_BillboardMatrix;
    mutable Float4x4  m_ProjectionMatrix;
    mutable BvFrustum m_Frustum;
    ECameraProjection m_Projection{CAMERA_PROJ_PERSPECTIVE_FOV_Y_ASPECT_RATIO};
    mutable bool      m_bViewMatrixDirty{true};
    mutable bool      m_bProjectionDirty{true};
    mutable bool      m_bFrustumDirty{true};
};
