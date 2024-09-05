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

#include "NavMeshAreaComponent.h"
#include <Hork/World/DebugRenderer.h>
#include <Hork/Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawNavMeshAreas("com_DrawNavMeshAreas"_s, "0"_s, CVAR_CHEAT);

BvAxisAlignedBox NavMeshAreaComponent::CalcBoundingBox() const
{
    switch (m_Shape)
    {
        case NavMeshAreaShape::Box:
        {
            Float3 worldPosition = GetOwner()->GetWorldPosition();
            return {worldPosition - m_HalfExtents, worldPosition + m_HalfExtents};
        }
        case NavMeshAreaShape::Cylinder:
        {
            Float3 worldPosition = GetOwner()->GetWorldPosition();

            float radius = GetCylinderRadius();
            float halfHeight = GetHeight() * 0.5f;

            return BvAxisAlignedBox({worldPosition.X - radius, worldPosition.Y - halfHeight, worldPosition.Z - radius},
                {worldPosition.X + radius, worldPosition.Y + halfHeight, worldPosition.Z + radius});
        }
        case NavMeshAreaShape::ConvexVolume:
        {
            if (m_VolumeContour.IsEmpty())
            {
                return {
                    {0, 0, 0},
                    {0, 0, 0}};
            }

            BvAxisAlignedBox boundingBox;
            boundingBox.Mins[0] = m_VolumeContour[0][0];
            boundingBox.Mins[2] = m_VolumeContour[0][1];
            boundingBox.Maxs[0] = m_VolumeContour[0][0];
            boundingBox.Maxs[2] = m_VolumeContour[0][1];
            for (Float2 const* pVert = &m_VolumeContour[1]; pVert < &m_VolumeContour.ToPtr()[m_VolumeContour.Size()]; pVert++)
            {
                boundingBox.Mins[0] = Math::Min(boundingBox.Mins[0], pVert->X);
                boundingBox.Mins[2] = Math::Min(boundingBox.Mins[2], pVert->Y);
                boundingBox.Maxs[0] = Math::Max(boundingBox.Maxs[0], pVert->X);
                boundingBox.Maxs[2] = Math::Max(boundingBox.Maxs[2], pVert->Y);
            }
            boundingBox.Mins[1] = -m_HalfExtents.Y;
            boundingBox.Maxs[1] = m_HalfExtents.Y;

            Float3 worldPosition = GetOwner()->GetWorldPosition();
            boundingBox.Mins += worldPosition;
            boundingBox.Maxs += worldPosition;

            return boundingBox;
        }
    }
    return {};
}

void NavMeshAreaComponent::DrawDebug(DebugRenderer& renderer)
{
    if (!com_DrawNavMeshAreas)
        return;

    renderer.SetDepthTest(false);
    renderer.SetColor(Color4::sBlue());
    switch (m_Shape)
    {
        case NavMeshAreaShape::Box:
            renderer.DrawBox(GetOwner()->GetWorldPosition(), m_HalfExtents);
            break;
        case NavMeshAreaShape::Cylinder:
            renderer.DrawCylinder(GetOwner()->GetWorldPosition(), Float3x3::sIdentity(), GetCylinderRadius(), GetHeight());
            break;
        case NavMeshAreaShape::ConvexVolume:
        {
            Float3 worldPosition = GetOwner()->GetWorldPosition();
            int count = m_VolumeContour.Size();
            for (int i = 0; i < count; ++i)
            {
                Float3 p0, p1;
                p0.X = m_VolumeContour[i].X;
                p0.Z = m_VolumeContour[i].Y;
                p1.X = m_VolumeContour[(i + 1) % count].X;
                p1.Z = m_VolumeContour[(i + 1) % count].Y;

                p0.Y = -m_HalfExtents.Y;
                p1.Y = -m_HalfExtents.Y;
                renderer.DrawLine(worldPosition + p0, worldPosition + p1);

                p0.Y = m_HalfExtents.Y;
                p1.Y = m_HalfExtents.Y;
                renderer.DrawLine(worldPosition + p0, worldPosition + p1);

                p0.Y = -m_HalfExtents.Y;
                p1.X = p0.X;
                p1.Y = m_HalfExtents.Y;
                p1.Z = p0.Z;
                renderer.DrawLine(worldPosition + p0, worldPosition + p1);
            }
            break;
        }
    }
}

HK_NAMESPACE_END
