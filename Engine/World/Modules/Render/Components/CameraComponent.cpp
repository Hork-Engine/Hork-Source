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

#include <Engine/Renderer/RenderDefs.h>
#include <Engine/World/World.h>
#include <Engine/World/DebugRenderer.h>
#include <Engine/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawCameraFrustum("com_DrawCameraFrustum"s, "0"s, CVAR_CHEAT);

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

void CameraComponent::SetAspectRatio(float aspectRatio)
{
    if (m_AspectRatio != aspectRatio)
    {
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
            fovY = std::atan2(1.0f, m_AspectRatio / std::tan(fovX * 0.5f)) * 2.0f;
            break;
        case CameraProjection::PerspectiveFovYWithAspectRatio:
            fovY = Math::Radians(m_FovY);
            fovX = std::atan(std::tan(fovY * 0.5f) * m_AspectRatio) * 2.0f;
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

void CameraComponent::MakeOrthoRect(float aspectRatio, float zoom, Float2& mins, Float2& maxs)
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

void CameraComponent::MakeClusterProjectionMatrix(Float4x4& projectionMatrix) const
{
    // TODO: if ( ClusterProjectionDirty ...

    switch (m_Projection)
    {
        case CameraProjection::OrthoRect:
            projectionMatrix = Float4x4::OrthoRevCC(m_OrthoMins, m_OrthoMaxs, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR);
            break;
        case CameraProjection::OrthoZoomWithAspectRatio: {
            Float2 orthoMins, orthoMaxs;
            CameraComponent::MakeOrthoRect(m_AspectRatio, 1.0f / m_OrthoZoom, orthoMins, orthoMaxs);
            projectionMatrix = Float4x4::OrthoRevCC(orthoMins, orthoMaxs, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR);
            break;
        }
        case CameraProjection::PerspectiveFovProvided:
            projectionMatrix = Float4x4::PerspectiveRevCC(Math::Radians(m_FovX), Math::Radians(m_FovY), FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR);
            break;
        case CameraProjection::PerspectiveFovXWithAspectRatio:
            projectionMatrix = Float4x4::PerspectiveRevCC(Math::Radians(m_FovX), m_AspectRatio, 1.0f, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR);
            break;
        case CameraProjection::PerspectiveFovYWithAspectRatio:
            projectionMatrix = Float4x4::PerspectiveRevCC_Y(Math::Radians(m_FovY), m_AspectRatio, 1.0f, FRUSTUM_CLUSTER_ZNEAR, FRUSTUM_CLUSTER_ZFAR);
            break;
    }
}

Float4x4 const& CameraComponent::GetProjectionMatrix() const
{
    if (m_ProjectionDirty)
    {
        switch (m_Projection)
        {
            case CameraProjection::OrthoRect:
                m_ProjectionMatrix = Float4x4::OrthoRevCC(m_OrthoMins, m_OrthoMaxs, m_ZNear, m_ZFar);
                break;
            case CameraProjection::OrthoZoomWithAspectRatio: {
                Float2 orthoMins, orthoMaxs;
                MakeOrthoRect(m_AspectRatio, 1.0f / m_OrthoZoom, orthoMins, orthoMaxs);
                m_ProjectionMatrix = Float4x4::OrthoRevCC(orthoMins, orthoMaxs, m_ZNear, m_ZFar);
                break;
            }
            case CameraProjection::PerspectiveFovProvided:
                m_ProjectionMatrix = Float4x4::PerspectiveRevCC(Math::Radians(m_FovX), Math::Radians(m_FovY), m_ZNear, m_ZFar);
                break;
            case CameraProjection::PerspectiveFovXWithAspectRatio:
                m_ProjectionMatrix = Float4x4::PerspectiveRevCC(Math::Radians(m_FovX), m_AspectRatio, 1.0f, m_ZNear, m_ZFar);
                break;
            case CameraProjection::PerspectiveFovYWithAspectRatio:
                m_ProjectionMatrix = Float4x4::PerspectiveRevCC_Y(Math::Radians(m_FovY), m_AspectRatio, 1.0f, m_ZNear, m_ZFar);
                break;
        }

        m_ProjectionDirty = false;
    }

    return m_ProjectionMatrix;
}

//void CameraComponent::MakeRay(float normalizedX, float normalizedY, Float3& rayStart, Float3& rayEnd) const
//{
//    // Update projection matrix
//    GetProjectionMatrix();
//
//    // Update view matrix
//    GetViewMatrix();
//
//    // TODO: cache ModelViewProjectionInversed
//    Float4x4 ModelViewProjectionInversed = (m_ProjectionMatrix * m_ViewMatrix).Inversed();
//    // TODO: try to optimize with m_ViewMatrix.ViewInverseFast() * m_ProjectionMatrix.ProjectionInverseFast()
//
//    MakeRay(ModelViewProjectionInversed, normalizedX, normalizedY, rayStart, rayEnd);
//}

void CameraComponent::MakeRay(Float4x4 const& modelViewProjectionInversed, float normalizedX, float normalizedY, Float3& rayStart, Float3& rayEnd)
{
    float x = 2.0f * normalizedX - 1.0f;
    float y = 2.0f * normalizedY - 1.0f;
#if 0
    Float4 near(x, y, 1, 1.0f);
    Float4 far(x, y, 0, 1.0f);
    rayStart.X = modelViewProjectionInversed[0][0] * near[0] + modelViewProjectionInversed[1][0] * near[1] + modelViewProjectionInversed[2][0] * near[2] + modelViewProjectionInversed[3][0];
    rayStart.Y = modelViewProjectionInversed[0][1] * near[0] + modelViewProjectionInversed[1][1] * near[1] + modelViewProjectionInversed[2][1] * near[2] + modelViewProjectionInversed[3][1];
    rayStart.Z = modelViewProjectionInversed[0][2] * near[0] + modelViewProjectionInversed[1][2] * near[1] + modelViewProjectionInversed[2][2] * near[2] + modelViewProjectionInversed[3][2];
    rayStart /= (modelViewProjectionInversed[0][3] * near[0] + modelViewProjectionInversed[1][3] * near[1] + modelViewProjectionInversed[2][3] * near[2] + modelViewProjectionInversed[3][3]);
    rayEnd.X = modelViewProjectionInversed[0][0] * far[0] + modelViewProjectionInversed[1][0] * far[1] + modelViewProjectionInversed[2][0] * far[2] + modelViewProjectionInversed[3][0];
    rayEnd.Y = modelViewProjectionInversed[0][1] * far[0] + modelViewProjectionInversed[1][1] * far[1] + modelViewProjectionInversed[2][1] * far[2] + modelViewProjectionInversed[3][1];
    rayEnd.Z = modelViewProjectionInversed[0][2] * far[0] + modelViewProjectionInversed[1][2] * far[1] + modelViewProjectionInversed[2][2] * far[2] + modelViewProjectionInversed[3][2];
    rayEnd /= (modelViewProjectionInversed[0][3] * far[0] + modelViewProjectionInversed[1][3] * far[1] + modelViewProjectionInversed[2][3] * far[2] + modelViewProjectionInversed[3][3]);
#else
    // Same code
    rayEnd.X = modelViewProjectionInversed[0][0] * x + modelViewProjectionInversed[1][0] * y + modelViewProjectionInversed[3][0];
    rayEnd.Y = modelViewProjectionInversed[0][1] * x + modelViewProjectionInversed[1][1] * y + modelViewProjectionInversed[3][1];
    rayEnd.Z = modelViewProjectionInversed[0][2] * x + modelViewProjectionInversed[1][2] * y + modelViewProjectionInversed[3][2];
    rayStart.X = rayEnd.X + modelViewProjectionInversed[2][0];
    rayStart.Y = rayEnd.Y + modelViewProjectionInversed[2][1];
    rayStart.Z = rayEnd.Z + modelViewProjectionInversed[2][2];
    float div = modelViewProjectionInversed[0][3] * x + modelViewProjectionInversed[1][3] * y + modelViewProjectionInversed[3][3];
    rayEnd /= div;
    div += modelViewProjectionInversed[2][3];
    rayStart /= div;
#endif
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
