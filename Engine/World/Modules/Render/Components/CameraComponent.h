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

#include <Engine/World/Component.h>
#include <Engine/World/TickFunction.h>

#include <Engine/Math/Quat.h>
#include <Engine/Geometry/BV/BvFrustum.h>

HK_NAMESPACE_BEGIN

enum class CameraProjection : uint8_t
{
    OrthoRect,
    OrthoZoomWithAspectRatio,
    PerspectiveFovProvided,
    PerspectiveFovXWithAspectRatio,
    PerspectiveFovYWithAspectRatio
};

class CameraComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    // TODO: Add offset position and offset rotation?
    //Float3                Position;
    //Quat                  Rotation;

    /// Set view projection
    void                    SetProjection(CameraProjection projection);

    /// Near clip distance
    void                    SetZNear(float zNear);

    /// Far clip distance
    void                    SetZFar(float zFar);

    /// Viewport aspect ratio. For example 4/3, 16/9
    void                    SetAspectRatio(float aspectRatio);

    /// Horizontal FOV for perspective projection
    void                    SetFovX(float fov);

    /// Vertical FOV for perspective projection
    void                    SetFovY(float fov);

    /// Rectangle for orthogonal projection
    void                    SetOrthoRect(Float2 const& mins, Float2 const& maxs);

    /// Zoom for orthogonal projection
    void                    SetOrthoZoom(float zoom);

    CameraProjection        GetProjection() const { return m_Projection; }

    bool                    IsPerspective() const { return m_Projection == CameraProjection::PerspectiveFovProvided || m_Projection == CameraProjection::PerspectiveFovXWithAspectRatio || m_Projection == CameraProjection::PerspectiveFovYWithAspectRatio; }

    bool                    IsOrthographic() const { return m_Projection == CameraProjection::OrthoRect || m_Projection == CameraProjection::OrthoZoomWithAspectRatio; }

    float                   GetZNear() const { return m_ZNear; }

    float                   GetZFar() const { return m_ZFar; }

    float                   GetAspectRatio() const { return m_AspectRatio; }

    float                   GetFovX() const { return m_FovX; }

    float                   GetFovY() const { return m_FovY; }

    /// Computes real camera field of view in radians for perspective projection
    void                    GetEffectiveFov(float& fovX, float& fovY) const;

    Float2 const&           GetOrthoMins() const { return m_OrthoMins; }

    Float2 const&           GetOrthoMaxs() const { return m_OrthoMaxs; }

    Float4x4 const&         GetProjectionMatrix() const;

    //Float4x4 const&       GetViewMatrix() const;

    //Float3x3 const&       GetBillboardMatrix() const;

    //BvFrustum const&      GetFrustum() const;

    /// NormalizedX = ScreenX / ScreenWidth, NormalizedY = ScreenY / ScreenHeight
    //void                  MakeRay(float normalizedX, float normalizedY, Float3& rayStart, Float3& rayEnd) const;

    static void             MakeRay(Float4x4 const& modelViewProjectionInversed, float normalizedX, float normalizedY, Float3& rayStart, Float3& rayEnd);

    /// Compute ortho rect based on aspect ratio and zoom
    static void             MakeOrthoRect(float aspectRatio, float zoom, Float2& mins, Float2& maxs);

    void                    MakeClusterProjectionMatrix(Float4x4& projectionMatrix /*, float clusterZNear, float clusterZFar*/) const;


    // Call to skip transform interpolation on this frame (useful for teleporting objects without smooth transition)
    void                    SkipInterpolation();

    void                    PostTransform();

    void                    BeginPlay();

    Float3 const&           GetPosition(int index) const {        return m_Position[index];    }

    Quat const&             GetRotation(int index) const {        return m_Rotation[index];    }

private:
    CameraProjection        m_Projection = CameraProjection::PerspectiveFovYWithAspectRatio;
    float                   m_FovX = 90.0f;
    float                   m_FovY = 90.0f;
    float                   m_ZNear = 0.04f;
    float                   m_ZFar = 99999.0f;
    float                   m_AspectRatio = 1.0f;
    Float2                  m_OrthoMins{-1, -1};
    Float2                  m_OrthoMaxs{1, 1};
    float                   m_OrthoZoom = 30;

    // Cache

    Float3                  m_Position[2];
    Quat                    m_Rotation[2];

    //mutable Float4x4      m_ViewMatrix;
    //mutable Float3x3      m_BillboardMatrix;
    mutable Float4x4        m_ProjectionMatrix;
    mutable BvFrustum       m_Frustum;
    //mutable bool          m_ViewMatrixDirty = true;
    mutable bool            m_ProjectionDirty = true;
    mutable bool            m_FrustumDirty = true;
};

namespace TickGroup_PostTransform
{
    template <>
    HK_INLINE void InitializeTickFunction<CameraComponent>(TickFunctionDesc& desc)
    {
        desc.TickEvenWhenPaused = true;
    }
}

HK_NAMESPACE_END
