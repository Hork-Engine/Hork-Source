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

#include <Engine/Renderer/RenderDefs.h>

#include <Engine/Geometry/BV/BvAxisAlignedBox.h>
#include <Engine/Geometry/BV/BvOrientedBox.h>
#include <Engine/Core/Containers/ArrayView.h>
#include <Engine/Core/Containers/Stack.h>

HK_NAMESPACE_BEGIN

using ArrayOfDebugVertices = TVector<DebugVertex>;
using ArrayOfDebugIndices  = TVector<unsigned short>;
using ArrayOfDebugDrawCmds = TVector<DebugDrawCmd>;

class DebugRenderer final : public Noncopyable
{
public:
    DebugRenderer();

    ~DebugRenderer();

    void Free();

    void Reset();

    void BeginRenderView(RenderViewData* InView, int InVisPass);

    void EndRenderView();

    RenderViewData const* GetRenderView() const { return m_pView; }

    void PushTransform(Float3x4 const& transform);
    void PopTransform();

    void SetDepthTest(bool _DepthTest);

    void SetColor(uint32_t _Color);

    void SetColor(Color4 const& _Color);

    void SetAlpha(float _Alpha);

    void SetRandomColors(bool bRandomVolors);

    void DrawPoint(Float3 const& _Position);

    void DrawPoints(Float3 const* _Points, int _NumPoints, int _Stride);

    void DrawPoints(TArrayView<Float3> _Points);

    void DrawLine(Float3 const& _P0, Float3 const& _P1);

    void DrawDottedLine(Float3 const& _P0, Float3 const& _P1, float _Step);

    void DrawLine(TArrayView<Float3> _Points, bool _Closed = false);

    void DrawConvexPoly(TArrayView<Float3> _Points, bool _TwoSided = false);

    void DrawTriangleSoup(Float3 const* _Points, int _NumPoints, int _Stride, unsigned int const* _Indices, int _NumIndices, bool _TwoSided = false);

    void DrawTriangleSoup(TArrayView<Float3> _Points, TArrayView<unsigned int> _Indices, bool _TwoSided = false);

    void DrawTriangleSoup(Float3 const* _Points, int _NumPoints, int _Stride, unsigned short const* _Indices, int _NumIndices, bool _TwoSided = false);

    void DrawTriangleSoup(TArrayView<Float3> _Points, TArrayView<unsigned short> _Indices, bool _TwoSided = false);

    void DrawTriangleSoupWireframe(Float3 const* _Points, int _Stride, unsigned int const* _Indices, int _NumIndices);

    void DrawTriangleSoupWireframe(TArrayView<Float3> _Points, TArrayView<unsigned int> _Indices);

    void DrawTriangleSoupWireframe(Float3 const* _Points, int _Stride, unsigned short const* _Indices, int _NumIndices);

    void DrawTriangleSoupWireframe(TArrayView<Float3> _Points, TArrayView<unsigned short> _Indices);

    void DrawTriangle(Float3 const& _P0, Float3 const& _P1, Float3 const& _P2, bool _TwoSided = false);

    void DrawTriangles(Float3 const* _Triangles, int _NumTriangles, int _Stride, bool _TwoSided = false);

    void DrawQuad(Float3 const& _P0, Float3 const& _P1, Float3 const& _P2, Float3 const& _P3, bool _TwoSided = false);

    void DrawBox(Float3 const& _Position, Float3 const& _HalfExtents);

    void DrawBoxFilled(Float3 const& _Position, Float3 const& _HalfExtents, bool _TwoSided = false);

    void DrawOrientedBox(Float3 const& _Position, Float3x3 const& _Orientation, Float3 const& _HalfExtents);

    void DrawOrientedBoxFilled(Float3 const& _Position, Float3x3 const& _Orientation, Float3 const& _HalfExtents, bool _TwoSided = false);

    void DrawSphere(Float3 const& _Position, float _Radius);

    void DrawOrientedSphere(Float3 const& _Position, Float3x3 const& _Orientation, float _Radius);

    // TODO: DrawSphereFilled

    void DrawSpherePatch(Float3 const& _Position, Float3 const& _Up, Float3 const& _Right, float _Radius, float _MinTh, float _MaxTh, float _MinPs, float _MaxPs, float _StepDegrees = 10.0f, bool _DrawCenter = true);

    void DrawCircle(Float3 const& _Position, Float3 const& _UpVector, float _Radius);

    void DrawCircleFilled(Float3 const& _Position, Float3 const& _UpVector, float _Radius, bool _TwoSided = false);

    void DrawCone(Float3 const& _Position, Float3x3 const& _Orientation, float _Radius, float _HalfAngle);

    // TODO: void DrawConeFilled( Float3 const & _Position, Float3x3 const & _Orientation, const float & _Radius, const float & _HalfAngle, bool _TwoSided = false );

    void DrawCylinder(Float3 const& _Position, Float3x3 const& _Orientation, float _Radius, float _Height);

    // TODO: void DrawCylinderFilled( Float3 const & _Position, Float3x3 const & _Orientation, const float & _Radius, const float & _Height, bool _TwoSided = false );

    void DrawCapsule(Float3 const& _Position, Float3x3 const& _Orientation, float _Radius, float _Height, int _UpAxis);

    // TODO: DrawCapsuleFilled

    void DrawAABB(BvAxisAlignedBox const& _AABB);

    void DrawOBB(BvOrientedBox const& _OBB);

    void DrawAxis(const Float3x4& _TransformMatrix, bool _Normalized);

    void DrawAxis(Float3 const& _Origin, Float3 const& _XVec, Float3 const& _YVec, Float3 const& _ZVec, Float3 const& _Scale = Float3(1));

    void DrawPlane(PlaneF const& _Plane, float _Length = 100.0f);

    void DrawPlane(Float3 const& _Normal, float _D, float _Length = 100.0f);

    void DrawPlaneFilled(PlaneF const& _Plane, float _Length = 100.0f, bool _TwoSided = false);

    void DrawPlaneFilled(Float3 const& _Normal, float _D, float _Length = 100.0f, bool _TwoSided = false);

    void SplitCommands();

    int CommandsCount() const { return m_Cmds.Size(); }

    int GetVisPass() const { return m_VisPass; }

    ArrayOfDebugVertices const& GetVertices() const { return m_Vertices; }
    ArrayOfDebugIndices const& GetIndices() const { return m_Indices; }
    ArrayOfDebugDrawCmds const& GetCmds() const { return m_Cmds; }

private:
    bool PrimitiveReserve(DBG_DRAW_CMD _CmdName, int _NumVertices, int _NumIndices, DebugDrawCmd** _Cmd, DebugVertex** _Verts, unsigned short** _Indices);

    RenderViewData*      m_pView{};
    ArrayOfDebugVertices m_Vertices;
    ArrayOfDebugIndices  m_Indices;
    ArrayOfDebugDrawCmds m_Cmds;
    uint32_t             m_CurrentColor{0xffffffff};
    int                  m_FirstVertex{};
    int                  m_FirstIndex{};
    int                  m_VisPass{};
    bool                 m_bDepthTest{};
    bool                 m_bSplit{};
    TStack<Float3x4, 4>  m_TransformStack;
    const uint32_t*      m_pColors{};
    size_t               m_ColorMask{1};
};

HK_NAMESPACE_END
