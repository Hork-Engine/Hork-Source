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

#include "DebugRenderer.h"
#include <Engine/Core/Color.h>

HK_NAMESPACE_BEGIN

#define PRIMITIVE_RESTART_INDEX 0xffff
#define MAX_PRIMITIVE_VERTS     0xfffe

DebugRenderer::DebugRenderer()
{
    m_TransformStack.Push(Float3x4::Identity());
    m_pColors = &m_CurrentColor;
}

void DebugRenderer::Purge()
{
    Reset();

    m_Vertices.Free();
    m_Indices.Free();
    m_Cmds.Free();
}

void DebugRenderer::Reset()
{
    m_CurrentColor = 0xffffffff;
    m_DepthTest = false;

    m_Vertices.Clear();
    m_Indices.Clear();
    m_Cmds.Clear();

    m_FirstVertex = 0;
    m_FirstIndex = 0;
    m_Split = false;

    m_pView = nullptr;

    m_TransformStack.Clear();
    m_TransformStack.Push(Float3x4::Identity());

    m_pColors = &m_CurrentColor;
    m_ColorMask = 0;
}

void DebugRenderer::BeginRenderView(RenderViewData* view, int visPass)
{
    HK_ASSERT(!m_pView);

    m_pView = view;
    m_pView->FirstDebugDrawCommand = CommandsCount();
    m_pView->DebugDrawCommandCount = 0;
    m_VisPass = visPass;
    SplitCommands();
}

void DebugRenderer::EndRenderView()
{
    HK_ASSERT(m_pView);

    m_pView->DebugDrawCommandCount = CommandsCount() - m_pView->FirstDebugDrawCommand;
    m_pView = nullptr;
}

void DebugRenderer::PushTransform(Float3x4 const& transform)
{
    m_TransformStack.Push(transform);
}

void DebugRenderer::PopTransform()
{
    m_TransformStack.Pop();
}

void DebugRenderer::SetDepthTest(bool depthTest)
{
    m_DepthTest = depthTest;
}

void DebugRenderer::SetColor(uint32_t color)
{
    m_CurrentColor = color;
}

void DebugRenderer::SetColor(Color4 const& color)
{
    m_CurrentColor = color.GetDWord();
}

void DebugRenderer::SetAlpha(float alpha)
{
    m_CurrentColor &= 0x00ffffff;
    m_CurrentColor |= Math::Clamp(Math::ToIntFast(alpha * 255), 0, 255) << 24;
}

namespace
{

uint32_t RandomColors[8] =
    {
        0xff0000ff,
        0xff00ff00,
        0xffff0000,
        0xff00ffff,
        0xffff00ff,
        0xffffff00,
        0xff883399,
        0xff789abc,
};

}

void DebugRenderer::EnableRandomColors(bool enable)
{
    if (enable)
    {
        m_pColors = RandomColors;
        m_ColorMask = HK_ARRAY_SIZE(RandomColors) - 1;
    }
    else
    {
        m_pColors = &m_CurrentColor;
        m_ColorMask = 0;
    }
}

void DebugRenderer::SplitCommands()
{
    m_Split = true;
}

bool DebugRenderer::PrimitiveReserve(DBG_DRAW_CMD name, int numVertices, int numIndices, DebugDrawCmd** pcmd, DebugVertex** verts, uint16_t** indices)
{
    if (numVertices <= 0 || numIndices <= 0)
    {
        // Empty primitive
        return false;
    }

    if (numVertices > MAX_PRIMITIVE_VERTS)
    {
        // TODO: split into several primitives
        LOG("DebugRenderer::PrimitiveReserve: primitive has too many vertices\n");
        return false;
    }

    if (!m_Cmds.IsEmpty())
    {
        DebugDrawCmd& cmd = m_Cmds.Last();

        if (cmd.NumVertices + numVertices > MAX_PRIMITIVE_VERTS)
        {
            SplitCommands();
        }
    }

    m_Vertices.Resize(m_Vertices.Size() + numVertices);
    m_Indices.Resize(m_Indices.Size() + numIndices);

    *verts = m_Vertices.ToPtr() + m_FirstVertex;
    *indices = m_Indices.ToPtr() + m_FirstIndex;

    if (m_Cmds.IsEmpty() || m_Split)
    {
        // Create first cmd or split
        DebugDrawCmd& cmd = m_Cmds.Add();
        cmd.Type = name;
        cmd.FirstVertex = m_FirstVertex;
        cmd.FirstIndex = m_FirstIndex;
        cmd.NumVertices = 0;
        cmd.NumIndices = 0;
        m_Split = false;
    }
    else if (m_Cmds.Last().NumIndices == 0)
    {
        // If last cmd has no indices, use it
        DebugDrawCmd& cmd = m_Cmds.Last();
        cmd.Type = name;
        cmd.FirstVertex = m_FirstVertex;
        cmd.FirstIndex = m_FirstIndex;
        cmd.NumVertices = 0;
    }
    else if (m_Cmds.Last().Type != name)
    {
        // If last cmd has other type, create a new cmd
        DebugDrawCmd& cmd = m_Cmds.Add();
        cmd.Type = name;
        cmd.FirstVertex = m_FirstVertex;
        cmd.FirstIndex = m_FirstIndex;
        cmd.NumVertices = 0;
        cmd.NumIndices = 0;
    }

    *pcmd = &m_Cmds.Last();

    m_FirstVertex += numVertices;
    m_FirstIndex += numIndices;

    return true;
}

void DebugRenderer::DrawPoint(Float3 const& position)
{
    DBG_DRAW_CMD cmdName = m_DepthTest ? DBG_DRAW_CMD_POINTS_DEPTH_TEST : DBG_DRAW_CMD_POINTS;
    DebugDrawCmd* cmd;
    DebugVertex* verts;
    uint16_t* indices;

    if (PrimitiveReserve(cmdName, 1, 1, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        verts->Position = transform * position;
        verts->Color = m_pColors[cmd->NumVertices & m_ColorMask];

        indices[0] = cmd->NumVertices;

        cmd->NumVertices++;
        cmd->NumIndices++;
    }
}

void DebugRenderer::DrawPoints(Float3 const* points, int numPoints, size_t stride)
{
    DBG_DRAW_CMD cmdName = m_DepthTest ? DBG_DRAW_CMD_POINTS_DEPTH_TEST : DBG_DRAW_CMD_POINTS;
    DebugDrawCmd* cmd;
    DebugVertex* verts;
    uint16_t* indices;

    if (PrimitiveReserve(cmdName, numPoints, numPoints, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        byte* pPoints = (byte*)points;

        for (int i = 0; i < numPoints; i++, verts++, indices++)
        {
            verts->Position = transform * *(Float3*)pPoints;
            verts->Color = m_pColors[i & m_ColorMask];
            *indices = cmd->NumVertices + i;

            pPoints += stride;
        }

        cmd->NumVertices += numPoints;
        cmd->NumIndices += numPoints;
    }
}

void DebugRenderer::DrawPoints(ArrayView<Float3> points)
{
    DrawPoints(points.ToPtr(), points.Size(), sizeof(Float3));
}

void DebugRenderer::DrawLine(Float3 const& p0, Float3 const& p1)
{
    DBG_DRAW_CMD cmdName = m_DepthTest ? DBG_DRAW_CMD_LINES_DEPTH_TEST : DBG_DRAW_CMD_LINES;
    DebugDrawCmd* cmd;
    DebugVertex* verts;
    uint16_t* indices;

    if (PrimitiveReserve(cmdName, 2, 3, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        verts[0].Position = transform * p0;
        verts[0].Color = m_pColors[cmd->NumVertices & m_ColorMask];
        verts[1].Position = transform * p1;
        verts[1].Color = m_pColors[cmd->NumVertices & m_ColorMask];

        indices[0] = cmd->NumVertices;
        indices[1] = cmd->NumVertices + 1;
        indices[2] = PRIMITIVE_RESTART_INDEX;

        cmd->NumVertices += 2;
        cmd->NumIndices += 3;
    }
}

void DebugRenderer::DrawDottedLine(Float3 const& p0, Float3 const& p1, float step)
{
    Float3 vector = p1 - p0;
    float len = vector.Length();
    Float3 dir = vector * (1.0f / len);
    float position = step * 0.5f;

    while (position < len)
    {
        float nextPosition = position + step;
        if (nextPosition > len)
        {
            nextPosition = len;
        }
        DrawLine(p0 + dir * position, p0 + dir * nextPosition);
        position = nextPosition + step;
    }
}

void DebugRenderer::DrawLine(ArrayView<Float3> points, bool closed)
{
    if (points.Size() < 2)
    {
        return;
    }

    int numIndices = closed ? points.Size() + 2 : points.Size() + 1;

    DBG_DRAW_CMD cmdName = m_DepthTest ? DBG_DRAW_CMD_LINES_DEPTH_TEST : DBG_DRAW_CMD_LINES;
    DebugDrawCmd* cmd;
    DebugVertex* verts;
    uint16_t* indices;

    if (PrimitiveReserve(cmdName, points.Size(), numIndices, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        for (int i = 0; i < points.Size(); i++, verts++, indices++)
        {
            verts->Position = transform * points[i];
            verts->Color = m_pColors[cmd->NumVertices & m_ColorMask];
            *indices = cmd->NumVertices + i;
        }

        if (closed)
        {
            *indices++ = cmd->NumVertices;
        }

        *indices = PRIMITIVE_RESTART_INDEX;

        cmd->NumVertices += points.Size();
        cmd->NumIndices += numIndices;
    }
}

void DebugRenderer::DrawConvexPoly(ArrayView<Float3> points, bool twoSided)
{
    if (points.Size() < 3)
    {
        return;
    }

    int numTriangles = points.Size() - 2;
    int numIndices = numTriangles * 3;

    if (twoSided)
    {
        numIndices <<= 1;
    }

    DBG_DRAW_CMD cmdName = m_DepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    DebugDrawCmd* cmd;
    DebugVertex* verts;
    uint16_t* indices;

    if (PrimitiveReserve(cmdName, points.Size(), numIndices, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        uint32_t color = m_pColors[cmd->NumVertices & m_ColorMask];

        for (int i = 0; i < points.Size(); i++, verts++)
        {
            verts->Position = transform * points[i];
            verts->Color = color;
        }

        for (int i = 0; i < numTriangles; i++)
        {
            *indices++ = cmd->NumVertices + 0;
            *indices++ = cmd->NumVertices + i + 1;
            *indices++ = cmd->NumVertices + i + 2;
        }

        if (twoSided)
        {
            for (int i = numTriangles - 1; i >= 0; i--)
            {
                *indices++ = cmd->NumVertices + 0;
                *indices++ = cmd->NumVertices + i + 2;
                *indices++ = cmd->NumVertices + i + 1;
            }
        }

        cmd->NumVertices += points.Size();
        cmd->NumIndices += numIndices;
    }
}

void DebugRenderer::DrawTriangleSoup(Float3 const* points, int numPoints, size_t stride, uint32_t const* _indices, int numIndices, bool twoSided)
{
    int totalIndices = numIndices;

    if (twoSided)
    {
        totalIndices <<= 1;
    }

    DBG_DRAW_CMD cmdName = m_DepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    DebugDrawCmd* cmd;
    DebugVertex* verts;
    uint16_t* indices;

    if (PrimitiveReserve(cmdName, numPoints, totalIndices, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        byte* pPoints = (byte*)points;

        for (int i = 0; i < numPoints; i++, verts++)
        {
            verts->Position = transform * *(Float3*)pPoints;
            verts->Color = m_pColors[(i / 3) & m_ColorMask];

            //#define VISUALIE_VERTICES
#ifdef VISUALIE_VERTICES
            verts->Color = Color4(Float4(i % 255, i % 255, i % 255, 255) / 255.0f).GetDWord();
#endif

            pPoints += stride;
        }

        for (int i = 0; i < numIndices; i++)
        {
            *indices++ = cmd->NumVertices + _indices[i];
        }

        if (twoSided)
        {
            _indices += numIndices;
            for (int i = numIndices - 1; i >= 0; i--)
            {
                *indices++ = cmd->NumVertices + *--_indices;
            }
        }

        cmd->NumVertices += numPoints;
        cmd->NumIndices += totalIndices;
    }
}

void DebugRenderer::DrawTriangleSoup(ArrayView<Float3> points, ArrayView<uint32_t> indices, bool twoSided)
{
    DrawTriangleSoup(points.ToPtr(), points.Size(), sizeof(Float3), indices.ToPtr(), indices.Size(), twoSided);
}

void DebugRenderer::DrawTriangleSoup(Float3 const* points, int numPoints, size_t stride, uint16_t const* _indices, int numIndices, bool twoSided)
{
    int totalIndices = numIndices;

    if (twoSided)
    {
        totalIndices <<= 1;
    }

    DBG_DRAW_CMD cmdName = m_DepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    DebugDrawCmd* cmd;
    DebugVertex* verts;
    uint16_t* indices;

    if (PrimitiveReserve(cmdName, numPoints, totalIndices, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        byte* pPoints = (byte*)points;

        for (int i = 0; i < numPoints; i++, verts++)
        {
            verts->Position = transform * *(Float3*)pPoints;
            verts->Color = m_pColors[(i / 3) & m_ColorMask];

            //#define VISUALIE_VERTICES
#ifdef VISUALIE_VERTICES
            verts->Color = Color4(Float4(i % 255, i % 255, i % 255, 255) / 255.0f).GetDWord();
#endif

            pPoints += stride;
        }

        for (int i = 0; i < numIndices; i++)
        {
            *indices++ = cmd->NumVertices + _indices[i];
        }

        if (twoSided)
        {
            _indices += numIndices;
            for (int i = numIndices - 1; i >= 0; i--)
            {
                *indices++ = cmd->NumVertices + *--_indices;
            }
        }

        cmd->NumVertices += numPoints;
        cmd->NumIndices += totalIndices;
    }
}

void DebugRenderer::DrawTriangleSoup(ArrayView<Float3> points, ArrayView<uint16_t> indices, bool twoSided)
{
    DrawTriangleSoup(points.ToPtr(), points.Size(), sizeof(Float3), indices.ToPtr(), indices.Size(), twoSided);
}

void DebugRenderer::DrawTriangleSoupWireframe(Float3 const* points, size_t stride, uint32_t const* indices, int numIndices)
{
    Float3 triangle[3];
    byte* pPoints = (byte*)points;
    for (int i = 0; i < numIndices; i += 3)
    {
        triangle[0] = *(Float3*)(pPoints + stride * indices[i + 0]);
        triangle[1] = *(Float3*)(pPoints + stride * indices[i + 1]);
        triangle[2] = *(Float3*)(pPoints + stride * indices[i + 2]);
        DrawLine(triangle, true);
    }
}

void DebugRenderer::DrawTriangleSoupWireframe(ArrayView<Float3> points, ArrayView<uint32_t> indices)
{
    DrawTriangleSoupWireframe(points.ToPtr(), sizeof(Float3), indices.ToPtr(), indices.Size());
}

void DebugRenderer::DrawTriangleSoupWireframe(Float3 const* points, size_t stride, uint16_t const* indices, int numIndices)
{
    Float3 triangle[3];
    byte* pPoints = (byte*)points;
    for (int i = 0; i < numIndices; i += 3)
    {
        triangle[0] = *(Float3*)(pPoints + stride * indices[i + 0]);
        triangle[1] = *(Float3*)(pPoints + stride * indices[i + 1]);
        triangle[2] = *(Float3*)(pPoints + stride * indices[i + 2]);
        DrawLine(triangle, true);
    }
}

void DebugRenderer::DrawTriangleSoupWireframe(ArrayView<Float3> points, ArrayView<uint16_t> indices)
{
    DrawTriangleSoupWireframe(points.ToPtr(), sizeof(Float3), indices.ToPtr(), indices.Size());
}

void DebugRenderer::DrawTriangle(Float3 const& p0, Float3 const& p1, Float3 const& p2, bool twoSided)
{
    Float3 points[] = {p0, p1, p2};
    DrawConvexPoly(points, twoSided);
}

void DebugRenderer::DrawTriangles(Float3 const* triangles, int numTriangles, size_t stride, bool twoSided)
{
    int numPoints = numTriangles * 3;
    int numIndices = numTriangles * 3;
    int totalIndices = twoSided ? numIndices << 1 : numIndices;
    DBG_DRAW_CMD cmdName = m_DepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    DebugDrawCmd* cmd;
    DebugVertex* verts;
    uint16_t* indices;

    if (PrimitiveReserve(cmdName, numPoints, totalIndices, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        byte* pPoints = (byte*)triangles;

        for (int i = 0; i < numPoints; i++, verts++)
        {
            verts->Position = transform * *(Float3*)pPoints;
            verts->Color = m_pColors[(i / 3) & m_ColorMask];

            pPoints += stride;
        }

        for (int i = 0; i < numIndices; i++)
        {
            *indices++ = cmd->NumVertices + i;
        }

        if (twoSided)
        {
            for (int i = numIndices - 1; i >= 0; i--)
            {
                *indices++ = cmd->NumVertices + i;
            }
        }

        cmd->NumVertices += numPoints;
        cmd->NumIndices += totalIndices;
    }
}

void DebugRenderer::DrawQuad(Float3 const& p0, Float3 const& p1, Float3 const& p2, Float3 const& p3, bool twoSided)
{
    Float3 points[] = {p0, p1, p2, p3};
    DrawConvexPoly(points, twoSided);
}

void DebugRenderer::DrawBox(Float3 const& position, Float3 const& halfExtents)
{
    Float3 points[4] = {
        Float3(-halfExtents.X, halfExtents.Y, -halfExtents.Z) + position,
        Float3(halfExtents.X, halfExtents.Y, -halfExtents.Z) + position,
        Float3(halfExtents.X, halfExtents.Y, halfExtents.Z) + position,
        Float3(-halfExtents.X, halfExtents.Y, halfExtents.Z) + position};
    Float3 points2[4] = {
        Float3(-halfExtents.X, -halfExtents.Y, -halfExtents.Z) + position,
        Float3(halfExtents.X, -halfExtents.Y, -halfExtents.Z) + position,
        Float3(halfExtents.X, -halfExtents.Y, halfExtents.Z) + position,
        Float3(-halfExtents.X, -halfExtents.Y, halfExtents.Z) + position};

    // top face
    DrawLine(points, true);

    // bottom face
    DrawLine(points2, true);

    // Edges
    DrawLine(points[0], points2[0]);
    DrawLine(points[1], points2[1]);
    DrawLine(points[2], points2[2]);
    DrawLine(points[3], points2[3]);
}

void DebugRenderer::DrawBoxFilled(Float3 const& position, Float3 const& halfExtents, bool twoSided)
{
    Float3 points[8] = {
        Float3(-halfExtents.X, halfExtents.Y, -halfExtents.Z) + position,
        Float3(halfExtents.X, halfExtents.Y, -halfExtents.Z) + position,
        Float3(halfExtents.X, halfExtents.Y, halfExtents.Z) + position,
        Float3(-halfExtents.X, halfExtents.Y, halfExtents.Z) + position,
        Float3(-halfExtents.X, -halfExtents.Y, -halfExtents.Z) + position,
        Float3(halfExtents.X, -halfExtents.Y, -halfExtents.Z) + position,
        Float3(halfExtents.X, -halfExtents.Y, halfExtents.Z) + position,
        Float3(-halfExtents.X, -halfExtents.Y, halfExtents.Z) + position};

    uint16_t indices[36] = {0, 3, 2, 2, 1, 0, 7, 4, 5, 5, 6, 7, 3, 7, 6, 6, 2, 3, 2, 6, 5, 5, 1, 2, 1, 5, 4, 4, 0, 1, 0, 4, 7, 7, 3, 0};

    DrawTriangleSoup(points, indices, twoSided);
}

void DebugRenderer::DrawOrientedBox(Float3 const& position, Float3x3 const& orientation, Float3 const& halfExtents)
{
    Float3 points[4] = {
        orientation * Float3(-halfExtents.X, halfExtents.Y, -halfExtents.Z) + position,
        orientation * Float3(halfExtents.X, halfExtents.Y, -halfExtents.Z) + position,
        orientation * Float3(halfExtents.X, halfExtents.Y, halfExtents.Z) + position,
        orientation * Float3(-halfExtents.X, halfExtents.Y, halfExtents.Z) + position};
    Float3 points2[4] = {
        orientation * Float3(-halfExtents.X, -halfExtents.Y, -halfExtents.Z) + position,
        orientation * Float3(halfExtents.X, -halfExtents.Y, -halfExtents.Z) + position,
        orientation * Float3(halfExtents.X, -halfExtents.Y, halfExtents.Z) + position,
        orientation * Float3(-halfExtents.X, -halfExtents.Y, halfExtents.Z) + position};

    // top face
    DrawLine(points, true);

    // bottom face
    DrawLine(points2, true);

    // Edges
    DrawLine(points[0], points2[0]);
    DrawLine(points[1], points2[1]);
    DrawLine(points[2], points2[2]);
    DrawLine(points[3], points2[3]);
}

void DebugRenderer::DrawOrientedBoxFilled(Float3 const& position, Float3x3 const& orientation, Float3 const& halfExtents, bool twoSided)
{
    Float3 points[8] = {
        orientation * Float3(-halfExtents.X, halfExtents.Y, -halfExtents.Z) + position,
        orientation * Float3(halfExtents.X, halfExtents.Y, -halfExtents.Z) + position,
        orientation * Float3(halfExtents.X, halfExtents.Y, halfExtents.Z) + position,
        orientation * Float3(-halfExtents.X, halfExtents.Y, halfExtents.Z) + position,
        orientation * Float3(-halfExtents.X, -halfExtents.Y, -halfExtents.Z) + position,
        orientation * Float3(halfExtents.X, -halfExtents.Y, -halfExtents.Z) + position,
        orientation * Float3(halfExtents.X, -halfExtents.Y, halfExtents.Z) + position,
        orientation * Float3(-halfExtents.X, -halfExtents.Y, halfExtents.Z) + position};

    uint16_t indices[36] = {0, 3, 2, 2, 1, 0, 7, 4, 5, 5, 6, 7, 3, 7, 6, 6, 2, 3, 2, 6, 5, 5, 1, 2, 1, 5, 4, 4, 0, 1, 0, 4, 7, 7, 3, 0};

    DrawTriangleSoup(points, indices, twoSided);
}

void DebugRenderer::DrawSphere(Float3 const& position, float radius)
{
    DrawOrientedSphere(position, Float3x3::Identity(), radius);
}

void DebugRenderer::DrawOrientedSphere(Float3 const& position, Float3x3 const& orientation, float radius)
{
    const float stepDegrees = 30.0f;
    DrawSpherePatch(position, orientation[1], orientation[0], radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false);
    DrawSpherePatch(position, orientation[1], -orientation[0], radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false);
}

void DebugRenderer::DrawSpherePatch(Float3 const& position, Float3 const& up, Float3 const& right, float radius, float minTh, float maxTh, float minPs, float maxPs, float stepDegrees, bool drawCenter)
{
    // This function is based on code from btDebugDraw
    Float3 vA[74];
    Float3 vB[74];
    Float3 *pvA = vA, *pvB = vB, *pT;
    Float3 npole = position + up * radius;
    Float3 spole = position - up * radius;
    Float3 arcStart;
    float step = Math::Radians(stepDegrees);
    Float3 backVec = Math::Cross(up, right);
    bool drawN = false;
    bool drawS = false;
    if (minTh <= -Math::_HALF_PI)
    {
        minTh = -Math::_HALF_PI + step;
        drawN = true;
    }
    if (maxTh >= Math::_HALF_PI)
    {
        maxTh = Math::_HALF_PI - step;
        drawS = true;
    }
    if (minTh > maxTh)
    {
        minTh = -Math::_HALF_PI + step;
        maxTh = Math::_HALF_PI - step;
        drawN = drawS = true;
    }
    int n_hor = (int)((maxTh - minTh) / step) + 1;
    if (n_hor < 2) n_hor = 2;
    float step_h = (maxTh - minTh) / float(n_hor - 1);
    bool isClosed = false;
    if (minPs > maxPs)
    {
        minPs = -Math::_PI + step;
        maxPs = Math::_PI;
        isClosed = true;
    }
    else if (maxPs - minPs >= Math::_2PI)
    {
        isClosed = true;
    }
    else
    {
        isClosed = false;
    }
    int n_vert = (int)((maxPs - minPs) / step) + 1;
    if (n_vert < 2) n_vert = 2;
    float step_v = (maxPs - minPs) / float(n_vert - 1);
    for (int i = 0; i < n_hor; i++)
    {
        float th = minTh + float(i) * step_h;
        float sth;
        float cth;
        Math::SinCos(th, sth, cth);
        sth *= radius;
        cth *= radius;
        for (int j = 0; j < n_vert; j++)
        {
            float psi = minPs + float(j) * step_v;
            float sps;
            float cps;
            Math::SinCos(psi, sps, cps);
            pvB[j] = position + cth * cps * right + cth * sps * backVec + sth * up;
            if (i)
            {
                DrawLine(pvA[j], pvB[j]);
            }
            else if (drawS)
            {
                DrawLine(spole, pvB[j]);
            }
            if (j)
            {
                DrawLine(pvB[j - 1], pvB[j]);
            }
            else
            {
                arcStart = pvB[j];
            }
            if ((i == (n_hor - 1)) && drawN)
            {
                DrawLine(npole, pvB[j]);
            }

            if (drawCenter)
            {
                if (isClosed)
                {
                    if (j == (n_vert - 1))
                    {
                        DrawLine(arcStart, pvB[j]);
                    }
                }
                else
                {
                    if ((!i || i == n_hor - 1) && (!j || j == n_vert - 1))
                    {
                        DrawLine(position, pvB[j]);
                    }
                }
            }
        }
        pT = pvA;
        pvA = pvB;
        pvB = pT;
    }
}

void DebugRenderer::DrawCircle(Float3 const& position, Float3 const& up, float radius)
{
    const int NumCirclePoints = 32;

    const Float3 v = up.Perpendicular() * radius;

    Float3 points[NumCirclePoints];

    points[0] = position + v;
    for (int i = 1; i < NumCirclePoints; ++i)
    {
        const float angle = Math::_2PI / NumCirclePoints * i;
        points[i] = position + Float3x3::RotationAroundNormal(angle, up) * v;
    }

    DrawLine(points, true);
}

void DebugRenderer::DrawCircleFilled(Float3 const& position, Float3 const& up, float radius, bool twoSided)
{
    const int NumCirclePoints = 32;

    const Float3 v = up.Perpendicular() * radius;

    Float3 points[NumCirclePoints];

    points[0] = position + v;
    for (int i = 1; i < NumCirclePoints; ++i)
    {
        const float angle = Math::_2PI / NumCirclePoints * i;
        points[i] = position + Float3x3::RotationAroundNormal(angle, up) * v;
    }

    DrawConvexPoly(points, twoSided);
}

void DebugRenderer::DrawCone(Float3 const& position, Float3x3 const& orientation, float radius, float halfAngleDegrees)
{
    const Float3 coneDirection = -orientation[2];
    const float halfAngle = Math::Clamp(Math::Radians(halfAngleDegrees), 0.0f, Math::_HALF_PI);
    const float R = radius / Math::Max(0.001f, Math::Cos(halfAngle));
    const Float3 v = Float3x3::RotationAroundNormal(halfAngle, orientation[0]) * coneDirection * R;

    const int NumCirclePoints = 32;
    Float3 points[NumCirclePoints];

    points[0] = position + v;
    for (int i = 1; i < NumCirclePoints; ++i)
    {
        const float angle = Math::_2PI / NumCirclePoints * i;
        points[i] = position + Float3x3::RotationAroundNormal(angle, coneDirection) * v;
    }

    // Draw cone circle
    DrawLine(points, true);

    // Draw cone faces (rays)
    for (int i = 0; i < NumCirclePoints; i += 2)
    {
        DrawLine(position, points[i]);
    }
}

void DebugRenderer::DrawCylinder(Float3 const& position, Float3x3 const& orientation, float radius, float height)
{
    const Float3 upVector = orientation[1] * height;
    const Float3 v = orientation[0] * radius;
    const Float3 pos = position - orientation[1] * (height * 0.5f);

    const int NumCirclePoints = 32;
    Float3 points[NumCirclePoints];

    points[0] = pos + v;
    for (int i = 1; i < NumCirclePoints; ++i)
    {
        const float angle = Math::_2PI / NumCirclePoints * i;
        points[i] = pos + Float3x3::RotationAroundNormal(angle, orientation[1]) * v;
    }

    // Draw bottom circle
    DrawLine(points, true);

    // Draw faces (rays)
    for (int i = 0; i < NumCirclePoints; i += 2)
    {
        DrawLine(points[i], points[i] + upVector);
        points[i] += upVector;
        points[i + 1] += upVector;
    }

    // Draw top circle
    DrawLine(points, true);
}

void DebugRenderer::DrawCapsule(Float3 const& position, Float3x3 const& orientation, float radius, float height, int upAxis)
{
    HK_ASSERT(upAxis >= 0 && upAxis < 3);

    const int stepDegrees = 30;
    const float halfHeight = height * 0.5f;

    Float3 capStart(0.0f);
    capStart[upAxis] = -halfHeight;

    Float3 capEnd(0.0f);
    capEnd[upAxis] = halfHeight;

    Float3 up = orientation.GetRow((upAxis + 1) % 3);
    Float3 axis = orientation.GetRow(upAxis);

    DrawSpherePatch(orientation * capStart + position, up, -axis, radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false);
    DrawSpherePatch(orientation * capEnd + position, up, axis, radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false);

    for (int angle = 0; angle < 360; angle += stepDegrees)
    {
        float r = Math::Radians(float(angle));
        capEnd[(upAxis + 1) % 3] = capStart[(upAxis + 1) % 3] = Math::Sin(r) * radius;
        capEnd[(upAxis + 2) % 3] = capStart[(upAxis + 2) % 3] = Math::Cos(r) * radius;
        DrawLine(position + orientation * capStart, position + orientation * capEnd);
    }
}

void DebugRenderer::DrawAABB(BvAxisAlignedBox const& aabb)
{
    DrawBox(aabb.Center(), aabb.HalfSize());
}

void DebugRenderer::DrawOBB(BvOrientedBox const& obb)
{
    DrawOrientedBox(obb.Center, obb.Orient, obb.HalfSize);
}

void DebugRenderer::DrawAxis(Float3x4 const& transformMatrix, bool normalized)
{
    Float3 Origin(transformMatrix[0][3], transformMatrix[1][3], transformMatrix[2][3]);
    Float3 XVec(transformMatrix[0][0], transformMatrix[1][0], transformMatrix[2][0]);
    Float3 YVec(transformMatrix[0][1], transformMatrix[1][1], transformMatrix[2][1]);
    Float3 ZVec(transformMatrix[0][2], transformMatrix[1][2], transformMatrix[2][2]);

    if (normalized)
    {
        XVec.NormalizeSelf();
        YVec.NormalizeSelf();
        ZVec.NormalizeSelf();
    }

    SetColor(Color4(1, 0, 0, 1));
    DrawLine(Origin, Origin + XVec);
    SetColor(Color4(0, 1, 0, 1));
    DrawLine(Origin, Origin + YVec);
    SetColor(Color4(0, 0, 1, 1));
    DrawLine(Origin, Origin + ZVec);
}

void DebugRenderer::DrawAxis(Float3 const& origin, Float3 const& xVec, Float3 const& yVec, Float3 const& zVec, Float3 const& scale)
{
    SetColor(Color4(1, 0, 0, 1));
    DrawLine(origin, origin + xVec * scale.X);
    SetColor(Color4(0, 1, 0, 1));
    DrawLine(origin, origin + yVec * scale.Y);
    SetColor(Color4(0, 0, 1, 1));
    DrawLine(origin, origin + zVec * scale.Z);
}

void DebugRenderer::DrawPlane(PlaneF const& plane, float length)
{
    DrawPlane(plane.Normal, plane.D, length);
}

void DebugRenderer::DrawPlane(Float3 const& normal, float d, float length)
{
    Float3 xvec, yvec, center;

    normal.ComputeBasis(xvec, yvec);

    center = normal * -d;

    Float3 points[4] = {
        center + (xvec + yvec) * length,
        center - (xvec - yvec) * length,
        center - (xvec + yvec) * length,
        center + (xvec - yvec) * length};

    DrawLine(points[0], points[2]);
    DrawLine(points[1], points[3]);
    DrawLine(points, true);
}

void DebugRenderer::DrawPlaneFilled(PlaneF const& plane, float length, bool twoSided)
{
    DrawPlaneFilled(plane.Normal, plane.D, length);
}

void DebugRenderer::DrawPlaneFilled(Float3 const& normal, float d, float length, bool twoSided)
{
    Float3 xvec, yvec, center;

    normal.ComputeBasis(xvec, yvec);

    center = normal * d;

    Float3 points[4] = {
        center + (xvec + yvec) * length,
        center - (xvec - yvec) * length,
        center - (xvec + yvec) * length,
        center + (xvec - yvec) * length};

    DrawConvexPoly(points, twoSided);
}

HK_NAMESPACE_END
