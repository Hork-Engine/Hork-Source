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
    m_bDepthTest = false;

    m_Vertices.Clear();
    m_Indices.Clear();
    m_Cmds.Clear();

    m_FirstVertex = 0;
    m_FirstIndex = 0;
    m_bSplit = false;

    m_pView = nullptr;

    m_TransformStack.Clear();
    m_TransformStack.Push(Float3x4::Identity());

    m_pColors = &m_CurrentColor;
    m_ColorMask = 0;
}

void DebugRenderer::BeginRenderView(RenderViewData* InView, int InVisPass)
{
    HK_ASSERT(!m_pView);

    m_pView = InView;
    m_pView->FirstDebugDrawCommand = CommandsCount();
    m_pView->DebugDrawCommandCount = 0;
    m_VisPass = InVisPass;
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

void DebugRenderer::SetDepthTest(bool _DepthTest)
{
    m_bDepthTest = _DepthTest;
}

void DebugRenderer::SetColor(uint32_t _Color)
{
    m_CurrentColor = _Color;
}

void DebugRenderer::SetColor(Color4 const& _Color)
{
    m_CurrentColor = _Color.GetDWord();
}

void DebugRenderer::SetAlpha(float _Alpha)
{
    m_CurrentColor &= 0x00ffffff;
    m_CurrentColor |= Math::Clamp(Math::ToIntFast(_Alpha * 255), 0, 255) << 24;
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

void DebugRenderer::SetRandomColors(bool bRandomVolors)
{
    if (bRandomVolors)
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
    m_bSplit = true;
}

bool DebugRenderer::PrimitiveReserve(DBG_DRAW_CMD _CmdName, int _NumVertices, int _NumIndices, DebugDrawCmd** _Cmd, DebugVertex** _Verts, uint16_t** _Indices)
{
    if (_NumVertices <= 0 || _NumIndices <= 0)
    {
        // Empty primitive
        return false;
    }

    if (_NumVertices > MAX_PRIMITIVE_VERTS)
    {
        // TODO: split to several primitives
        LOG("DebugRenderer::PrimitiveReserve: primitive has too many vertices\n");
        return false;
    }

    if (!m_Cmds.IsEmpty())
    {
        DebugDrawCmd& cmd = m_Cmds.Last();

        if (cmd.NumVertices + _NumVertices > MAX_PRIMITIVE_VERTS)
        {
            SplitCommands();
        }
    }

    m_Vertices.Resize(m_Vertices.Size() + _NumVertices);
    m_Indices.Resize(m_Indices.Size() + _NumIndices);

    *_Verts = m_Vertices.ToPtr() + m_FirstVertex;
    *_Indices = m_Indices.ToPtr() + m_FirstIndex;

    if (m_Cmds.IsEmpty() || m_bSplit)
    {
        // Create first cmd or split
        DebugDrawCmd& cmd = m_Cmds.Add();
        cmd.Type           = _CmdName;
        cmd.FirstVertex    = m_FirstVertex;
        cmd.FirstIndex     = m_FirstIndex;
        cmd.NumVertices    = 0;
        cmd.NumIndices     = 0;
        m_bSplit           = false;
    }
    else if (m_Cmds.Last().NumIndices == 0)
    {
        // If last cmd has no indices, use it
        DebugDrawCmd& cmd = m_Cmds.Last();
        cmd.Type           = _CmdName;
        cmd.FirstVertex    = m_FirstVertex;
        cmd.FirstIndex     = m_FirstIndex;
        cmd.NumVertices    = 0;
    }
    else if (m_Cmds.Last().Type != _CmdName)
    {
        // If last cmd has other type, create a new cmd
        DebugDrawCmd& cmd = m_Cmds.Add();
        cmd.Type           = _CmdName;
        cmd.FirstVertex    = m_FirstVertex;
        cmd.FirstIndex     = m_FirstIndex;
        cmd.NumVertices    = 0;
        cmd.NumIndices     = 0;
    }

    *_Cmd = &m_Cmds.Last();

    m_FirstVertex += _NumVertices;
    m_FirstIndex += _NumIndices;

    return true;
}

void DebugRenderer::DrawPoint(Float3 const& _Position)
{
    DBG_DRAW_CMD   cmdName = m_bDepthTest ? DBG_DRAW_CMD_POINTS_DEPTH_TEST : DBG_DRAW_CMD_POINTS;
    DebugDrawCmd*  cmd;
    DebugVertex*   verts;
    uint16_t*      indices;

    if (PrimitiveReserve(cmdName, 1, 1, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        verts->Position = transform * _Position;
        verts->Color    = m_pColors[cmd->NumVertices & m_ColorMask];

        indices[0] = cmd->NumVertices;

        cmd->NumVertices++;
        cmd->NumIndices++;
    }
}

void DebugRenderer::DrawPoints(Float3 const* _Points, int _NumPoints, int _Stride)
{
    DBG_DRAW_CMD   cmdName = m_bDepthTest ? DBG_DRAW_CMD_POINTS_DEPTH_TEST : DBG_DRAW_CMD_POINTS;
    DebugDrawCmd*  cmd;
    DebugVertex*   verts;
    uint16_t*      indices;

    if (PrimitiveReserve(cmdName, _NumPoints, _NumPoints, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        byte* pPoints = (byte*)_Points;

        for (int i = 0; i < _NumPoints; i++, verts++, indices++)
        {
            verts->Position = transform * *(Float3*)pPoints;
            verts->Color    = m_pColors[i & m_ColorMask];
            *indices        = cmd->NumVertices + i;

            pPoints += _Stride;
        }

        cmd->NumVertices += _NumPoints;
        cmd->NumIndices += _NumPoints;
    }
}

void DebugRenderer::DrawPoints(ArrayView<Float3> _Points)
{
    DrawPoints(_Points.ToPtr(), _Points.Size(), sizeof(Float3));
}

void DebugRenderer::DrawLine(Float3 const& _P0, Float3 const& _P1)
{
    DBG_DRAW_CMD   cmdName = m_bDepthTest ? DBG_DRAW_CMD_LINES_DEPTH_TEST : DBG_DRAW_CMD_LINES;
    DebugDrawCmd*  cmd;
    DebugVertex*   verts;
    uint16_t*      indices;

    if (PrimitiveReserve(cmdName, 2, 3, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        verts[0].Position = transform * _P0;
        verts[0].Color    = m_pColors[cmd->NumVertices & m_ColorMask];
        verts[1].Position = transform * _P1;
        verts[1].Color    = m_pColors[cmd->NumVertices & m_ColorMask];

        indices[0] = cmd->NumVertices;
        indices[1] = cmd->NumVertices + 1;
        indices[2] = PRIMITIVE_RESTART_INDEX;

        cmd->NumVertices += 2;
        cmd->NumIndices += 3;
    }
}

void DebugRenderer::DrawDottedLine(Float3 const& _P0, Float3 const& _P1, float _Step)
{
    Float3 vector   = _P1 - _P0;
    float  len      = vector.Length();
    Float3 dir      = vector * (1.0f / len);
    float  position = _Step * 0.5f;

    while (position < len)
    {
        float nextPosition = position + _Step;
        if (nextPosition > len)
        {
            nextPosition = len;
        }
        DrawLine(_P0 + dir * position, _P0 + dir * nextPosition);
        position = nextPosition + _Step;
    }
}

void DebugRenderer::DrawLine(ArrayView<Float3> _Points, bool _Closed)
{
    if (_Points.Size() < 2)
    {
        return;
    }

    int numIndices = _Closed ? _Points.Size() + 2 : _Points.Size() + 1;

    DBG_DRAW_CMD   cmdName = m_bDepthTest ? DBG_DRAW_CMD_LINES_DEPTH_TEST : DBG_DRAW_CMD_LINES;
    DebugDrawCmd*  cmd;
    DebugVertex*   verts;
    uint16_t*      indices;

    if (PrimitiveReserve(cmdName, _Points.Size(), numIndices, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        for (int i = 0; i < _Points.Size(); i++, verts++, indices++)
        {
            verts->Position = transform * _Points[i];
            verts->Color    = m_pColors[cmd->NumVertices & m_ColorMask];
            *indices        = cmd->NumVertices + i;
        }

        if (_Closed)
        {
            *indices++ = cmd->NumVertices;
        }

        *indices = PRIMITIVE_RESTART_INDEX;

        cmd->NumVertices += _Points.Size();
        cmd->NumIndices += numIndices;
    }
}

void DebugRenderer::DrawConvexPoly(ArrayView<Float3> _Points, bool _TwoSided)
{
    if (_Points.Size() < 3)
    {
        return;
    }

    int numTriangles = _Points.Size() - 2;
    int numIndices   = numTriangles * 3;

    if (_TwoSided)
    {
        numIndices <<= 1;
    }

    DBG_DRAW_CMD   cmdName = m_bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    DebugDrawCmd*  cmd;
    DebugVertex*   verts;
    uint16_t*      indices;

    if (PrimitiveReserve(cmdName, _Points.Size(), numIndices, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        uint32_t color = m_pColors[cmd->NumVertices & m_ColorMask];

        for (int i = 0; i < _Points.Size(); i++, verts++)
        {
            verts->Position = transform * _Points[i];
            verts->Color    = color;
        }

        for (int i = 0; i < numTriangles; i++)
        {
            *indices++ = cmd->NumVertices + 0;
            *indices++ = cmd->NumVertices + i + 1;
            *indices++ = cmd->NumVertices + i + 2;
        }

        if (_TwoSided)
        {
            for (int i = numTriangles - 1; i >= 0; i--)
            {
                *indices++ = cmd->NumVertices + 0;
                *indices++ = cmd->NumVertices + i + 2;
                *indices++ = cmd->NumVertices + i + 1;
            }
        }

        cmd->NumVertices += _Points.Size();
        cmd->NumIndices += numIndices;
    }
}

void DebugRenderer::DrawTriangleSoup(Float3 const* _Points, int _NumPoints, int _Stride, uint32_t const* _Indices, int _NumIndices, bool _TwoSided)
{
    int numIndices = _NumIndices;

    if (_TwoSided)
    {
        numIndices <<= 1;
    }

    DBG_DRAW_CMD   cmdName = m_bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    DebugDrawCmd*  cmd;
    DebugVertex*   verts;
    uint16_t*      indices;

    if (PrimitiveReserve(cmdName, _NumPoints, numIndices, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        byte* pPoints = (byte*)_Points;

        for (int i = 0; i < _NumPoints; i++, verts++)
        {
            verts->Position = transform * *(Float3*)pPoints;
            verts->Color    = m_pColors[(i/3) & m_ColorMask];

            //#define VISUALIE_VERTICES
#ifdef VISUALIE_VERTICES
            verts->Color = Color4(Float4(i % 255, i % 255, i % 255, 255) / 255.0f).GetDWord();
#endif

            pPoints += _Stride;
        }

        for (int i = 0; i < _NumIndices; i++)
        {
            *indices++ = cmd->NumVertices + _Indices[i];
        }

        if (_TwoSided)
        {
            _Indices += _NumIndices;
            for (int i = _NumIndices - 1; i >= 0; i--)
            {
                *indices++ = cmd->NumVertices + *--_Indices;
            }
        }

        cmd->NumVertices += _NumPoints;
        cmd->NumIndices += numIndices;
    }
}

void DebugRenderer::DrawTriangleSoup(ArrayView<Float3> _Points, ArrayView<uint32_t> _Indices, bool _TwoSided)
{
    DrawTriangleSoup(_Points.ToPtr(), _Points.Size(), sizeof(Float3), _Indices.ToPtr(), _Indices.Size(), _TwoSided);
}

void DebugRenderer::DrawTriangleSoup(Float3 const* _Points, int _NumPoints, int _Stride, uint16_t const* _Indices, int _NumIndices, bool _TwoSided)
{
    int numIndices = _NumIndices;

    if (_TwoSided)
    {
        numIndices <<= 1;
    }

    DBG_DRAW_CMD   cmdName = m_bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    DebugDrawCmd*  cmd;
    DebugVertex*   verts;
    uint16_t*      indices;

    if (PrimitiveReserve(cmdName, _NumPoints, numIndices, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        byte* pPoints = (byte*)_Points;

        for (int i = 0; i < _NumPoints; i++, verts++)
        {
            verts->Position = transform * *(Float3*)pPoints;
            verts->Color    = m_pColors[(i/3) & m_ColorMask];

            //#define VISUALIE_VERTICES
#ifdef VISUALIE_VERTICES
            verts->Color = Color4(Float4(i % 255, i % 255, i % 255, 255) / 255.0f).GetDWord();
#endif

            pPoints += _Stride;
        }

        for (int i = 0; i < _NumIndices; i++)
        {
            *indices++ = cmd->NumVertices + _Indices[i];
        }

        if (_TwoSided)
        {
            _Indices += _NumIndices;
            for (int i = _NumIndices - 1; i >= 0; i--)
            {
                *indices++ = cmd->NumVertices + *--_Indices;
            }
        }

        cmd->NumVertices += _NumPoints;
        cmd->NumIndices += numIndices;
    }
}

void DebugRenderer::DrawTriangleSoup(ArrayView<Float3> _Points, ArrayView<uint16_t> _Indices, bool _TwoSided)
{
    DrawTriangleSoup(_Points.ToPtr(), _Points.Size(), sizeof(Float3), _Indices.ToPtr(), _Indices.Size(), _TwoSided);
}

void DebugRenderer::DrawTriangleSoupWireframe(Float3 const* _Points, int _Stride, uint32_t const* _Indices, int _NumIndices)
{
    Float3 points[3];
    byte*  pPoints = (byte*)_Points;
    for (int i = 0; i < _NumIndices; i += 3)
    {
        points[0] = *(Float3*)(pPoints + _Stride * _Indices[i + 0]);
        points[1] = *(Float3*)(pPoints + _Stride * _Indices[i + 1]);
        points[2] = *(Float3*)(pPoints + _Stride * _Indices[i + 2]);
        DrawLine(points, true);
    }
}

void DebugRenderer::DrawTriangleSoupWireframe(ArrayView<Float3> _Points, ArrayView<uint32_t> _Indices)
{
    DrawTriangleSoupWireframe(_Points.ToPtr(), sizeof(Float3), _Indices.ToPtr(), _Indices.Size());
}

void DebugRenderer::DrawTriangleSoupWireframe(Float3 const* _Points, int _Stride, uint16_t const* _Indices, int _NumIndices)
{
    Float3 points[3];
    byte*  pPoints = (byte*)_Points;
    for (int i = 0; i < _NumIndices; i += 3)
    {
        points[0] = *(Float3*)(pPoints + _Stride * _Indices[i + 0]);
        points[1] = *(Float3*)(pPoints + _Stride * _Indices[i + 1]);
        points[2] = *(Float3*)(pPoints + _Stride * _Indices[i + 2]);
        DrawLine(points, true);
    }
}

void DebugRenderer::DrawTriangleSoupWireframe(ArrayView<Float3> _Points, ArrayView<uint16_t> _Indices)
{
    DrawTriangleSoupWireframe(_Points.ToPtr(), sizeof(Float3), _Indices.ToPtr(), _Indices.Size());
}

void DebugRenderer::DrawTriangle(Float3 const& _P0, Float3 const& _P1, Float3 const& _P2, bool _TwoSided)
{
    Float3 points[] = {_P0, _P1, _P2};
    DrawConvexPoly(points, _TwoSided);
}

void DebugRenderer::DrawTriangles(Float3 const* _Triangles, int _NumTriangles, int _Stride, bool _TwoSided)
{
    int            numPoints    = _NumTriangles * 3;
    int            numIndices   = _NumTriangles * 3;
    int            totalIndices = _TwoSided ? numIndices << 1 : numIndices;
    DBG_DRAW_CMD   cmdName      = m_bDepthTest ? DBG_DRAW_CMD_TRIANGLE_SOUP_DEPTH_TEST : DBG_DRAW_CMD_TRIANGLE_SOUP;
    DebugDrawCmd*  cmd;
    DebugVertex*   verts;
    uint16_t*      indices;

    if (PrimitiveReserve(cmdName, numPoints, totalIndices, &cmd, &verts, &indices))
    {
        Float3x4 const& transform = m_TransformStack.Top();

        byte* pPoints = (byte*)_Triangles;

        for (int i = 0; i < numPoints; i++, verts++)
        {
            verts->Position = transform * *(Float3*)pPoints;
            verts->Color    = m_pColors[(i/3) & m_ColorMask];

            pPoints += _Stride;
        }

        for (int i = 0; i < numIndices; i++)
        {
            *indices++ = cmd->NumVertices + i;
        }

        if (_TwoSided)
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

void DebugRenderer::DrawQuad(Float3 const& _P0, Float3 const& _P1, Float3 const& _P2, Float3 const& _P3, bool _TwoSided)
{
    Float3 points[] = {_P0, _P1, _P2, _P3};
    DrawConvexPoly(points, _TwoSided);
}

void DebugRenderer::DrawBox(Float3 const& _Position, Float3 const& _HalfExtents)
{
    Float3 points[4] = {
        Float3(-_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z) + _Position,
        Float3(_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z) + _Position,
        Float3(_HalfExtents.X, _HalfExtents.Y, _HalfExtents.Z) + _Position,
        Float3(-_HalfExtents.X, _HalfExtents.Y, _HalfExtents.Z) + _Position};
    Float3 points2[4] = {
        Float3(-_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z) + _Position,
        Float3(_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z) + _Position,
        Float3(_HalfExtents.X, -_HalfExtents.Y, _HalfExtents.Z) + _Position,
        Float3(-_HalfExtents.X, -_HalfExtents.Y, _HalfExtents.Z) + _Position};

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

void DebugRenderer::DrawBoxFilled(Float3 const& _Position, Float3 const& _HalfExtents, bool _TwoSided)
{
    Float3 points[8] = {
        Float3(-_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z) + _Position,
        Float3(_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z) + _Position,
        Float3(_HalfExtents.X, _HalfExtents.Y, _HalfExtents.Z) + _Position,
        Float3(-_HalfExtents.X, _HalfExtents.Y, _HalfExtents.Z) + _Position,
        Float3(-_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z) + _Position,
        Float3(_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z) + _Position,
        Float3(_HalfExtents.X, -_HalfExtents.Y, _HalfExtents.Z) + _Position,
        Float3(-_HalfExtents.X, -_HalfExtents.Y, _HalfExtents.Z) + _Position};

    uint16_t indices[36] = {0, 3, 2, 2, 1, 0, 7, 4, 5, 5, 6, 7, 3, 7, 6, 6, 2, 3, 2, 6, 5, 5, 1, 2, 1, 5, 4, 4, 0, 1, 0, 4, 7, 7, 3, 0};

    DrawTriangleSoup(points, indices, _TwoSided);
}

void DebugRenderer::DrawOrientedBox(Float3 const& _Position, Float3x3 const& _Orientation, Float3 const& _HalfExtents)
{
    Float3 points[4] = {
        _Orientation * Float3(-_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z) + _Position,
        _Orientation * Float3(_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z) + _Position,
        _Orientation * Float3(_HalfExtents.X, _HalfExtents.Y, _HalfExtents.Z) + _Position,
        _Orientation * Float3(-_HalfExtents.X, _HalfExtents.Y, _HalfExtents.Z) + _Position};
    Float3 points2[4] = {
        _Orientation * Float3(-_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z) + _Position,
        _Orientation * Float3(_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z) + _Position,
        _Orientation * Float3(_HalfExtents.X, -_HalfExtents.Y, _HalfExtents.Z) + _Position,
        _Orientation * Float3(-_HalfExtents.X, -_HalfExtents.Y, _HalfExtents.Z) + _Position};

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

void DebugRenderer::DrawOrientedBoxFilled(Float3 const& _Position, Float3x3 const& _Orientation, Float3 const& _HalfExtents, bool _TwoSided)
{
    Float3 points[8] = {
        _Orientation * Float3(-_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z) + _Position,
        _Orientation * Float3(_HalfExtents.X, _HalfExtents.Y, -_HalfExtents.Z) + _Position,
        _Orientation * Float3(_HalfExtents.X, _HalfExtents.Y, _HalfExtents.Z) + _Position,
        _Orientation * Float3(-_HalfExtents.X, _HalfExtents.Y, _HalfExtents.Z) + _Position,
        _Orientation * Float3(-_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z) + _Position,
        _Orientation * Float3(_HalfExtents.X, -_HalfExtents.Y, -_HalfExtents.Z) + _Position,
        _Orientation * Float3(_HalfExtents.X, -_HalfExtents.Y, _HalfExtents.Z) + _Position,
        _Orientation * Float3(-_HalfExtents.X, -_HalfExtents.Y, _HalfExtents.Z) + _Position};

    uint16_t indices[36] = {0, 3, 2, 2, 1, 0, 7, 4, 5, 5, 6, 7, 3, 7, 6, 6, 2, 3, 2, 6, 5, 5, 1, 2, 1, 5, 4, 4, 0, 1, 0, 4, 7, 7, 3, 0};

    DrawTriangleSoup(points, indices, _TwoSided);
}

void DebugRenderer::DrawSphere(Float3 const& _Position, float _Radius)
{
    DrawOrientedSphere(_Position, Float3x3::Identity(), _Radius);
}

void DebugRenderer::DrawOrientedSphere(Float3 const& _Position, Float3x3 const& _Orientation, float _Radius)
{
    const float stepDegrees = 30.0f;
    DrawSpherePatch(_Position, _Orientation[1], _Orientation[0], _Radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false);
    DrawSpherePatch(_Position, _Orientation[1], -_Orientation[0], _Radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false);
}

void DebugRenderer::DrawSpherePatch(Float3 const& _Position, Float3 const& _Up, Float3 const& _Right, float _Radius, float _MinTh, float _MaxTh, float _MinPs, float _MaxPs, float _StepDegrees, bool _DrawCenter)
{
    // This function is based on code from btDebugDraw
    Float3  vA[74];
    Float3  vB[74];
    Float3 *pvA = vA, *pvB = vB, *pT;
    Float3  npole = _Position + _Up * _Radius;
    Float3  spole = _Position - _Up * _Radius;
    Float3  arcStart;
    float   step    = Math::Radians(_StepDegrees);
    Float3  backVec = Math::Cross(_Up, _Right);
    bool    drawN   = false;
    bool    drawS   = false;
    if (_MinTh <= -Math::_HALF_PI)
    {
        _MinTh = -Math::_HALF_PI + step;
        drawN  = true;
    }
    if (_MaxTh >= Math::_HALF_PI)
    {
        _MaxTh = Math::_HALF_PI - step;
        drawS  = true;
    }
    if (_MinTh > _MaxTh)
    {
        _MinTh = -Math::_HALF_PI + step;
        _MaxTh = Math::_HALF_PI - step;
        drawN = drawS = true;
    }
    int n_hor = (int)((_MaxTh - _MinTh) / step) + 1;
    if (n_hor < 2) n_hor = 2;
    float step_h   = (_MaxTh - _MinTh) / float(n_hor - 1);
    bool  isClosed = false;
    if (_MinPs > _MaxPs)
    {
        _MinPs   = -Math::_PI + step;
        _MaxPs   = Math::_PI;
        isClosed = true;
    }
    else if (_MaxPs - _MinPs >= Math::_2PI)
    {
        isClosed = true;
    }
    else
    {
        isClosed = false;
    }
    int n_vert = (int)((_MaxPs - _MinPs) / step) + 1;
    if (n_vert < 2) n_vert = 2;
    float step_v = (_MaxPs - _MinPs) / float(n_vert - 1);
    for (int i = 0; i < n_hor; i++)
    {
        float th = _MinTh + float(i) * step_h;
        float sth;
        float cth;
        Math::SinCos(th, sth, cth);
        sth *= _Radius;
        cth *= _Radius;
        for (int j = 0; j < n_vert; j++)
        {
            float psi = _MinPs + float(j) * step_v;
            float sps;
            float cps;
            Math::SinCos(psi, sps, cps);
            pvB[j] = _Position + cth * cps * _Right + cth * sps * backVec + sth * _Up;
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

            if (_DrawCenter)
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
                        DrawLine(_Position, pvB[j]);
                    }
                }
            }
        }
        pT  = pvA;
        pvA = pvB;
        pvB = pT;
    }
}

void DebugRenderer::DrawCircle(Float3 const& _Position, Float3 const& _UpVector, float _Radius)
{
    const int NumCirclePoints = 32;

    const Float3 v = _UpVector.Perpendicular() * _Radius;

    Float3 points[NumCirclePoints];

    points[0] = _Position + v;
    for (int i = 1; i < NumCirclePoints; ++i)
    {
        const float Angle = Math::_2PI / NumCirclePoints * i;
        points[i]         = _Position + Float3x3::RotationAroundNormal(Angle, _UpVector) * v;
    }

    DrawLine(points, true);
}

void DebugRenderer::DrawCircleFilled(Float3 const& _Position, Float3 const& _UpVector, float _Radius, bool _TwoSided)
{
    const int NumCirclePoints = 32;

    const Float3 v = _UpVector.Perpendicular() * _Radius;

    Float3 points[NumCirclePoints];

    points[0] = _Position + v;
    for (int i = 1; i < NumCirclePoints; ++i)
    {
        const float Angle = Math::_2PI / NumCirclePoints * i;
        points[i]         = _Position + Float3x3::RotationAroundNormal(Angle, _UpVector) * v;
    }

    DrawConvexPoly(points, _TwoSided);
}

void DebugRenderer::DrawCone(Float3 const& _Position, Float3x3 const& _Orientation, float _Radius, float _HalfAngle)
{
    const Float3 coneDirection = -_Orientation[2];
    const float  halfAngle     = Math::Clamp(_HalfAngle, 0.0f, Math::_HALF_PI);
    const float  R             = _Radius / Math::Max(0.001f, Math::Cos(halfAngle));
    const Float3 v             = Float3x3::RotationAroundNormal(halfAngle, _Orientation[0]) * coneDirection * R;

    const int NumCirclePoints = 32;
    Float3    points[NumCirclePoints];

    points[0] = _Position + v;
    for (int i = 1; i < NumCirclePoints; ++i)
    {
        const float Angle = Math::_2PI / NumCirclePoints * i;
        points[i]         = _Position + Float3x3::RotationAroundNormal(Angle, coneDirection) * v;
    }

    // Draw cone circle
    DrawLine(points, true);

    // Draw cone faces (rays)
    for (int i = 0; i < NumCirclePoints; i += 2)
    {
        DrawLine(_Position, points[i]);
    }
}

void DebugRenderer::DrawCylinder(Float3 const& _Position, Float3x3 const& _Orientation, float _Radius, float _Height)
{
    const Float3 upVector = _Orientation[1] * _Height;
    const Float3 v        = _Orientation[0] * _Radius;
    const Float3 pos      = _Position - _Orientation[1] * (_Height * 0.5f);

    const int NumCirclePoints = 32;
    Float3    points[NumCirclePoints];

    points[0] = pos + v;
    for (int i = 1; i < NumCirclePoints; ++i)
    {
        const float Angle = Math::_2PI / NumCirclePoints * i;
        points[i]         = pos + Float3x3::RotationAroundNormal(Angle, _Orientation[1]) * v;
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

void DebugRenderer::DrawCapsule(Float3 const& _Position, Float3x3 const& _Orientation, float _Radius, float _Height, int _UpAxis)
{
    HK_ASSERT(_UpAxis >= 0 && _UpAxis < 3);

    const int   stepDegrees = 30;
    const float halfHeight  = _Height * 0.5f;

    Float3 capStart(0.0f);
    capStart[_UpAxis] = -halfHeight;

    Float3 capEnd(0.0f);
    capEnd[_UpAxis] = halfHeight;

    Float3 up   = _Orientation.GetRow((_UpAxis + 1) % 3);
    Float3 axis = _Orientation.GetRow(_UpAxis);

    DrawSpherePatch(_Orientation * capStart + _Position, up, -axis, _Radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false);
    DrawSpherePatch(_Orientation * capEnd + _Position, up, axis, _Radius, -Math::_HALF_PI, Math::_HALF_PI, -Math::_HALF_PI, Math::_HALF_PI, stepDegrees, false);

    for (int angle = 0; angle < 360; angle += stepDegrees)
    {
        float r                   = Math::Radians(float(angle));
        capEnd[(_UpAxis + 1) % 3] = capStart[(_UpAxis + 1) % 3] = Math::Sin(r) * _Radius;
        capEnd[(_UpAxis + 2) % 3] = capStart[(_UpAxis + 2) % 3] = Math::Cos(r) * _Radius;
        DrawLine(_Position + _Orientation * capStart, _Position + _Orientation * capEnd);
    }
}

void DebugRenderer::DrawAABB(BvAxisAlignedBox const& _AABB)
{
    DrawBox(_AABB.Center(), _AABB.HalfSize());
}

void DebugRenderer::DrawOBB(BvOrientedBox const& _OBB)
{
    DrawOrientedBox(_OBB.Center, _OBB.Orient, _OBB.HalfSize);
}

void DebugRenderer::DrawAxis(Float3x4 const& _TransformMatrix, bool _Normalized)
{
    Float3 Origin(_TransformMatrix[0][3], _TransformMatrix[1][3], _TransformMatrix[2][3]);
    Float3 XVec(_TransformMatrix[0][0], _TransformMatrix[1][0], _TransformMatrix[2][0]);
    Float3 YVec(_TransformMatrix[0][1], _TransformMatrix[1][1], _TransformMatrix[2][1]);
    Float3 ZVec(_TransformMatrix[0][2], _TransformMatrix[1][2], _TransformMatrix[2][2]);

    if (_Normalized)
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

void DebugRenderer::DrawAxis(Float3 const& _Origin, Float3 const& _XVec, Float3 const& _YVec, Float3 const& _ZVec, Float3 const& _Scale)
{
    SetColor(Color4(1, 0, 0, 1));
    DrawLine(_Origin, _Origin + _XVec * _Scale.X);
    SetColor(Color4(0, 1, 0, 1));
    DrawLine(_Origin, _Origin + _YVec * _Scale.Y);
    SetColor(Color4(0, 0, 1, 1));
    DrawLine(_Origin, _Origin + _ZVec * _Scale.Z);
}

void DebugRenderer::DrawPlane(PlaneF const& _Plane, float _Length)
{
    DrawPlane(_Plane.Normal, _Plane.D, _Length);
}

void DebugRenderer::DrawPlane(Float3 const& _Normal, float _D, float _Length)
{
    Float3 xvec, yvec, center;

    _Normal.ComputeBasis(xvec, yvec);

    center = _Normal * -_D;

    Float3 points[4] = {
        center + (xvec + yvec) * _Length,
        center - (xvec - yvec) * _Length,
        center - (xvec + yvec) * _Length,
        center + (xvec - yvec) * _Length};

    DrawLine(points[0], points[2]);
    DrawLine(points[1], points[3]);
    DrawLine(points, true);
}

void DebugRenderer::DrawPlaneFilled(PlaneF const& _Plane, float _Length, bool _TwoSided)
{
    DrawPlaneFilled(_Plane.Normal, _Plane.D, _Length);
}

void DebugRenderer::DrawPlaneFilled(Float3 const& _Normal, float _D, float _Length, bool _TwoSided)
{
    Float3 xvec, yvec, center;

    _Normal.ComputeBasis(xvec, yvec);

    center = _Normal * _D;

    Float3 points[4] = {
        center + (xvec + yvec) * _Length,
        center - (xvec - yvec) * _Length,
        center - (xvec + yvec) * _Length,
        center + (xvec - yvec) * _Length};

    DrawConvexPoly(points, _TwoSided);
}

HK_NAMESPACE_END
