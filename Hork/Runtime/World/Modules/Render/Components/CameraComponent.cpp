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

#include "CameraComponent.h"

#include <Hork/Renderer/RenderDefs.h>
#include <Hork/Runtime/World/World.h>
#include <Hork/Runtime/World/DebugRenderer.h>
#include <Hork/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawCameraFrustum("com_DrawCameraFrustum"_s, "0"_s, CVAR_CHEAT);

namespace
{
    float CalcFovXFromFovY(float fieldOfView, float aspectRatio)
    {
        return Math::Degrees(2.0f * std::atan(std::tan(Math::Radians(fieldOfView) * 0.5f) * aspectRatio));
    }
    float CalcFovYFromFovX(float fieldOfView, float aspectRatio)
    {
        return Math::Degrees(2.0f * std::atan(std::tan(Math::Radians(fieldOfView) * 0.5f) / aspectRatio));
    }
}

void CameraComponent::SetProjection(CameraProjection projection)
{
    if (m_Projection != projection)
    {
        m_Projection = projection;
        m_ProjectionDirty = true;
    }
}

void CameraComponent::SetZNear(float zNear)
{
    if (m_ZNear != zNear)
    {
        m_ZNear = zNear;
        m_ProjectionDirty = true;
    }
}

void CameraComponent::SetZFar(float zFar)
{
    if (m_ZFar != zFar)
    {
        m_ZFar = zFar;
        m_ProjectionDirty = true;
    }
}

void CameraComponent::SetFocalLength(float millimeters)
{
    const float SensorSizeMM = 24;  // 35mm camera has a 36x24mm wide frame size
    SetFovY(2.0f * std::atan(0.5f * SensorSizeMM / millimeters));
}

float CameraComponent::GetFocalLength() const
{
    const float SensorSizeMM = 24;
    return SensorSizeMM / std::tan(m_FovY * 0.5) * 0.5;
}

void CameraComponent::SetFovX(float fov)
{
    if (m_FovX != fov)
    {
        m_FovX = fov;
        m_ProjectionDirty = true;
    }
}

void CameraComponent::SetFovY(float fov)
{
    if (m_FovY != fov)
    {
        m_FovY = fov;
        m_ProjectionDirty = true;
    }
}

void CameraComponent::SetViewportPosition(Float2 const& viewportPos)
{
    m_ViewportPosition = viewportPos;
}

void CameraComponent::SetViewportSize(Float2 const& viewportSize, float aspectScale)
{
    float aspectRatio;

    if (viewportSize.X <= 0.0f || viewportSize.Y <= 0.0f)
        aspectRatio = 1;
    else
        aspectRatio = viewportSize.X / viewportSize.Y;

    aspectRatio *= aspectScale;

    if (m_ViewportSize != viewportSize || m_AspectRatio != aspectRatio)
    {
        m_ViewportSize = viewportSize;
        m_AspectRatio = aspectRatio;

        m_ProjectionDirty = true;
    }
}

void CameraComponent::GetEffectiveFov(float& fovX, float& fovY) const
{
    fovX = 0;
    fovY = 0;
    switch (m_Projection)
    {
        case CameraProjection::OrthoRect:
        case CameraProjection::OrthoZoomWithAspectRatio:
            break;
        case CameraProjection::PerspectiveFovProvided:
            fovX = Math::Radians(m_FovX);
            fovY = Math::Radians(m_FovY);
            break;
        case CameraProjection::PerspectiveFovXWithAspectRatio:
            fovX = Math::Radians(m_FovX);
            fovY = Math::Radians(CalcFovYFromFovX(m_FovX, m_AspectRatio));
            break;
        case CameraProjection::PerspectiveFovYWithAspectRatio:
            fovX = Math::Radians(CalcFovXFromFovY(m_FovY, m_AspectRatio));
            fovY = Math::Radians(m_FovY);
            break;
    }
}

void CameraComponent::SetOrthoRect(Float2 const& mins, Float2 const& maxs)
{
    m_OrthoMins = mins;
    m_OrthoMaxs = maxs;

    if (IsOrthographic())
    {
        m_ProjectionDirty = true;
    }
}

void CameraComponent::SetOrthoZoom(float zoom)
{
    m_OrthoZoom = zoom;

    if (IsOrthographic())
    {
        m_ProjectionDirty = true;
    }
}

void CameraComponent::sGetOrthoRect(float aspectRatio, float zoom, Float2& mins, Float2& maxs)
{
    if (aspectRatio > 0.0f)
    {
        const float Z = zoom != 0.0f ? 1.0f / zoom : 0.0f;
        maxs.X = Z;
        maxs.Y = Z / aspectRatio;
        mins = -maxs;
    }
    else
    {
        mins.X = -1;
        mins.Y = -1;
        maxs.X = 1;
        maxs.Y = 1;
    }
}

Float4x4 CameraComponent::GetClusterProjectionMatrix() const
{
    switch (m_Projection)
    {
        case CameraProjection::OrthoRect:
        {
            Float4x4::OrthoMatrixDesc desc = {};
            desc.Mins = m_OrthoMins;
            desc.Maxs = m_OrthoMaxs;
            desc.ZNear = FRUSTUM_CLUSTER_ZNEAR;
            desc.ZFar = FRUSTUM_CLUSTER_ZFAR;
            desc.ReversedDepth = true;
            return Float4x4::sGetOrthoMatrix(desc);
        }
        case CameraProjection::OrthoZoomWithAspectRatio:
        {
            Float4x4::OrthoMatrixDesc desc = {};
            desc.ZNear = FRUSTUM_CLUSTER_ZNEAR;
            desc.ZFar = FRUSTUM_CLUSTER_ZFAR;
            desc.ReversedDepth = true;
            CameraComponent::sGetOrthoRect(m_AspectRatio, 1.0f / m_OrthoZoom, desc.Mins, desc.Maxs);
            return Float4x4::sGetOrthoMatrix(desc);
        }
        case CameraProjection::PerspectiveFovProvided:
        {
            Float4x4::PerspectiveMatrixDesc2 desc = {};
            desc.FieldOfViewX = m_FovX;
            desc.FieldOfViewY = m_FovY;
            desc.ZNear = FRUSTUM_CLUSTER_ZNEAR;
            desc.ZFar = FRUSTUM_CLUSTER_ZFAR;
            return Float4x4::sGetPerspectiveMatrix(desc);
        }
        case CameraProjection::PerspectiveFovXWithAspectRatio:
        {
            Float4x4::PerspectiveMatrixDesc desc = {};
            desc.AspectRatio = m_AspectRatio;
            desc.FieldOfView = CalcFovYFromFovX(m_FovX, m_AspectRatio);
            desc.ZNear = FRUSTUM_CLUSTER_ZNEAR;
            desc.ZFar = FRUSTUM_CLUSTER_ZFAR;
            return Float4x4::sGetPerspectiveMatrix(desc);
        }
        case CameraProjection::PerspectiveFovYWithAspectRatio:
        {
            Float4x4::PerspectiveMatrixDesc desc = {};
            desc.AspectRatio = m_AspectRatio;
            desc.FieldOfView = m_FovY;
            desc.ZNear = FRUSTUM_CLUSTER_ZNEAR;
            desc.ZFar = FRUSTUM_CLUSTER_ZFAR;
            return Float4x4::sGetPerspectiveMatrix(desc);
        }
    }

    return Float4x4::sIdentity();
}

Float4x4 const& CameraComponent::GetProjectionMatrix() const
{
    if (m_ProjectionDirty)
    {
        switch (m_Projection)
        {
            case CameraProjection::OrthoRect:
            {
                Float4x4::OrthoMatrixDesc desc = {};
                desc.Mins = m_OrthoMins;
                desc.Maxs = m_OrthoMaxs;
                desc.ZNear = m_ZNear;
                desc.ZFar = m_ZFar;
                desc.ReversedDepth = true;
                m_ProjectionMatrix = Float4x4::sGetOrthoMatrix(desc);
                break;
            }
            case CameraProjection::OrthoZoomWithAspectRatio:
            {
                Float4x4::OrthoMatrixDesc desc = {};
                desc.ZNear = m_ZNear;
                desc.ZFar = m_ZFar;
                desc.ReversedDepth = true;
                sGetOrthoRect(m_AspectRatio, 1.0f / m_OrthoZoom, desc.Mins, desc.Maxs);
                m_ProjectionMatrix = Float4x4::sGetOrthoMatrix(desc);
                break;
            }
            case CameraProjection::PerspectiveFovProvided:
            {
                Float4x4::PerspectiveMatrixDesc2 desc = {};
                desc.FieldOfViewX = m_FovX;
                desc.FieldOfViewY = m_FovY;
                desc.ZNear = m_ZNear;
                desc.ZFar = m_ZFar;
                m_ProjectionMatrix = Float4x4::sGetPerspectiveMatrix(desc);
                break;
            }
            case CameraProjection::PerspectiveFovXWithAspectRatio:
            {
                Float4x4::PerspectiveMatrixDesc desc = {};
                desc.AspectRatio = m_AspectRatio;
                desc.FieldOfView = CalcFovYFromFovX(m_FovX, m_AspectRatio);
                desc.ZNear = m_ZNear;
                desc.ZFar = m_ZFar;
                m_ProjectionMatrix = Float4x4::sGetPerspectiveMatrix(desc);
                break;
            }
            case CameraProjection::PerspectiveFovYWithAspectRatio:
            {
                Float4x4::PerspectiveMatrixDesc desc = {};
                desc.AspectRatio = m_AspectRatio;
                desc.FieldOfView = m_FovY;
                desc.ZNear = m_ZNear;
                desc.ZFar = m_ZFar;
                m_ProjectionMatrix = Float4x4::sGetPerspectiveMatrix(desc);
                break;
            }
        }

        m_ProjectionDirty = false;
    }

    return m_ProjectionMatrix;
}

BvFrustum CameraComponent::GetFrustum() const
{
    BvFrustum frustum;
    frustum.FromMatrix(GetProjectionMatrix() * GetViewMatrix(), true);
    return frustum;
}

Float4x4 CameraComponent::GetViewMatrix() const
{
    Float3 worldPosition = GetOwner()->GetWorldPosition();
    Float3x3 worldRotation = GetOwner()->GetWorldRotation().ToMatrix3x3();

    Float3x3 basis = worldRotation.Transposed();
    Float3 origin = basis * (-worldPosition);

    return Float4x4(Float4(basis[0], 0.0f),
                    Float4(basis[1], 0.0f),
                    Float4(basis[2], 0.0f),
                    Float4(origin, 1.0f));
}

Float3x3 CameraComponent::GetBillboardMatrix() const
{
    return GetOwner()->GetWorldRotation().ToMatrix3x3();
}

Float2 CameraComponent::ScreenToViewportPoint(Float2 const& screenPoint) const
{
    if (m_ViewportSize.X <= 0 || m_ViewportSize.Y <= 0)
        return Float2(-1.0f);

    return (screenPoint - m_ViewportPosition) / m_ViewportSize;
}

Float2 CameraComponent::ViewportPointToScreen(Float2 const& viewportPoint) const
{
    if (m_ViewportSize.X <= 0 || m_ViewportSize.Y <= 0)
        return Float2(-1.0f);

    return viewportPoint * m_ViewportSize + m_ViewportPosition;
}

bool CameraComponent::ViewportPointToRay(Float2 const& viewportPoint, Float3& rayStart, Float3& rayDir) const
{
    if (viewportPoint.X < 0 || viewportPoint.Y < 0 || viewportPoint.X > 1 || viewportPoint.Y > 1)
    {
        // Point is outside of camera viewport
        return false;
    }

    auto proj = GetProjectionMatrix();
    auto view = GetViewMatrix();

    auto viewProj = proj * view;
    auto viewProjInversed = viewProj.Inversed();

    float x = 2.0f * viewportPoint.X - 1.0f;
    float y = 2.0f * viewportPoint.Y - 1.0f;

    Float3 rayEnd;
    rayEnd.X = viewProjInversed[0][0] * x + viewProjInversed[1][0] * y + viewProjInversed[3][0];
    rayEnd.Y = viewProjInversed[0][1] * x + viewProjInversed[1][1] * y + viewProjInversed[3][1];
    rayEnd.Z = viewProjInversed[0][2] * x + viewProjInversed[1][2] * y + viewProjInversed[3][2];
    rayStart.X = rayEnd.X + viewProjInversed[2][0];
    rayStart.Y = rayEnd.Y + viewProjInversed[2][1];
    rayStart.Z = rayEnd.Z + viewProjInversed[2][2];

    float div = viewProjInversed[0][3] * x + viewProjInversed[1][3] * y + viewProjInversed[3][3];
    rayEnd /= div;
    div += viewProjInversed[2][3];
    rayStart /= div;

    rayDir = rayEnd - rayStart;

    return true;
}

bool CameraComponent::ScreenPointToRay(Float2 const& screenPoint, Float3& rayStart, Float3& rayDir) const
{
    return ViewportPointToRay(ScreenToViewportPoint(screenPoint), rayStart, rayDir);
}

void CameraComponent::SkipInterpolation()
{
    m_Position[0] = m_Position[1] = GetOwner()->GetWorldPosition();
    m_Rotation[0] = m_Rotation[1] = GetOwner()->GetWorldRotation();
}

void CameraComponent::PostTransform()
{
    auto index = GetWorld()->GetTick().StateIndex;

    m_Position[index] = GetOwner()->GetWorldPosition();
    m_Rotation[index] = GetOwner()->GetWorldRotation();
}

void CameraComponent::BeginPlay()
{
    m_Position[0] = m_Position[1] = GetOwner()->GetWorldPosition();
    m_Rotation[0] = m_Rotation[1] = GetOwner()->GetWorldRotation();
}

void CameraComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawCameraFrustum)
    {
        Float3 vectorTR;
        Float3 vectorTL;
        Float3 vectorBR;
        Float3 vectorBL;
        Float3 origin = GetOwner()->GetWorldPosition();
        Float3 v[4];
        Float3 faces[4][3];
        float rayLength = 32;

        BvFrustum frustum = GetFrustum();

        frustum.CornerVector_TR(vectorTR);
        frustum.CornerVector_TL(vectorTL);
        frustum.CornerVector_BR(vectorBR);
        frustum.CornerVector_BL(vectorBL);

        v[0] = origin + vectorTR * rayLength;
        v[1] = origin + vectorBR * rayLength;
        v[2] = origin + vectorBL * rayLength;
        v[3] = origin + vectorTL * rayLength;

        // top
        faces[0][0] = origin;
        faces[0][1] = v[0];
        faces[0][2] = v[3];

        // left
        faces[1][0] = origin;
        faces[1][1] = v[3];
        faces[1][2] = v[2];

        // bottom
        faces[2][0] = origin;
        faces[2][1] = v[2];
        faces[2][2] = v[1];

        // right
        faces[3][0] = origin;
        faces[3][1] = v[1];
        faces[3][2] = v[0];

        renderer.SetDepthTest(true);

        renderer.SetColor(Color4(0, 1, 1, 1));
        renderer.DrawLine(origin, v[0]);
        renderer.DrawLine(origin, v[3]);
        renderer.DrawLine(origin, v[1]);
        renderer.DrawLine(origin, v[2]);
        renderer.DrawLine(v, true);

        renderer.SetColor(Color4(1, 1, 1, 0.3f));
        renderer.DrawTriangles(&faces[0][0], 4, sizeof(Float3), false);
        renderer.DrawConvexPoly(v, false);
    }
}

HK_NAMESPACE_END
