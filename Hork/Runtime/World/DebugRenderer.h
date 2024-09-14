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

#include <Hork/Renderer/RenderDefs.h>
#include <Hork/Geometry/BV/BvAxisAlignedBox.h>
#include <Hork/Geometry/BV/BvOrientedBox.h>
#include <Hork/Math/Plane.h>
#include <Hork/Core/Containers/ArrayView.h>
#include <Hork/Core/Containers/Stack.h>

HK_NAMESPACE_BEGIN

class DebugRenderer final : public Noncopyable
{
public:
    using DebugVertices =   Vector<DebugVertex>;
    using DebugIndices  =   Vector<uint16_t>;
    using DebugDrawCmds =   Vector<DebugDrawCmd>;

                            DebugRenderer();

    void                    Reset();
    void                    Purge();

    void                    BeginRenderView(Float3 const& viewPosition, int visPass);
    void                    EndRenderView(int& firstCommand, int& commandCount);

    Float3 const&           GetViewPosition() const;

    void                    PushTransform(Float3x4 const& transform);
    void                    PopTransform();

    void                    SetDepthTest(bool depthTest);
    void                    SetColor(uint32_t color);
    void                    SetColor(Color4 const& color);
    void                    SetAlpha(float alpha);
    void                    EnableRandomColors(bool enable);

    void                    DrawPoint(Float3 const& position);
    void                    DrawPoints(Float3 const* points, int numPoints, size_t stride);
    void                    DrawPoints(ArrayView<Float3> points);

    void                    DrawLine(Float3 const& p0, Float3 const& p1);
    void                    DrawDottedLine(Float3 const& p0, Float3 const& p1, float step);
    void                    DrawLine(ArrayView<Float3> points, bool closed = false);

    void                    DrawConvexPoly(ArrayView<Float3> points, bool twoSided = false);

    void                    DrawTriangleSoup(Float3 const* points, int numPoints, size_t stride, uint32_t const* indices, int numIndices, bool twoSided = false);
    void                    DrawTriangleSoup(ArrayView<Float3> points, ArrayView<uint32_t> indices, bool twoSided = false);
    void                    DrawTriangleSoup(Float3 const* points, int numPoints, size_t stride, uint16_t const* indices, int numIndices, bool twoSided = false);
    void                    DrawTriangleSoup(ArrayView<Float3> points, ArrayView<uint16_t> indices, bool twoSided = false);

    void                    DrawTriangleSoupWireframe(Float3 const* points, size_t stride, uint32_t const* indices, int numIndices);
    void                    DrawTriangleSoupWireframe(ArrayView<Float3> points, ArrayView<uint32_t> indices);
    void                    DrawTriangleSoupWireframe(Float3 const* points, size_t stride, uint16_t const* indices, int numIndices);
    void                    DrawTriangleSoupWireframe(ArrayView<Float3> points, ArrayView<uint16_t> indices);

    void                    DrawTriangle(Float3 const& p0, Float3 const& p1, Float3 const& p2, bool twoSided = false);
    void                    DrawTriangles(Float3 const* triangles, int numTriangles, size_t stride, bool twoSided = false);

    void                    DrawQuad(Float3 const& p0, Float3 const& p1, Float3 const& p2, Float3 const& p3, bool twoSided = false);

    void                    DrawBox(Float3 const& position, Float3 const& halfExtents);
    void                    DrawBoxFilled(Float3 const& position, Float3 const& halfExtents, bool twoSided = false);

    void                    DrawOrientedBox(Float3 const& position, Float3x3 const& orientation, Float3 const& halfExtents);
    void                    DrawOrientedBoxFilled(Float3 const& position, Float3x3 const& orientation, Float3 const& halfExtents, bool twoSided = false);

    void                    DrawSphere(Float3 const& position, float radius);
    void                    DrawOrientedSphere(Float3 const& position, Float3x3 const& orientation, float radius);
    void                    DrawSpherePatch(Float3 const& position, Float3 const& up, Float3 const& right, float radius, float minTh, float maxTh, float minPs, float maxPs, float stepDegrees = 10.0f, bool drawCenter = true);

    void                    DrawCircle(Float3 const& position, Float3 const& up, float radius);
    void                    DrawCircleFilled(Float3 const& position, Float3 const& up, float radius, bool twoSided = false);

    void                    DrawCone(Float3 const& position, Float3x3 const& orientation, float radius, float halfAngleDegrees);
    void                    DrawCylinder(Float3 const& position, Float3x3 const& orientation, float radius, float height);
    void                    DrawCapsule(Float3 const& position, Float3x3 const& orientation, float radius, float height, int upAxis);

    void                    DrawAABB(BvAxisAlignedBox const& aabb);
    void                    DrawOBB(BvOrientedBox const& obb);

    void                    DrawAxis(Float3x4 const& transformMatrix, bool normalized);
    void                    DrawAxis(Float3 const& origin, Float3 const& xVec, Float3 const& yVec, Float3 const& zVec, Float3 const& scale = Float3(1));

    void                    DrawPlane(PlaneF const& plane, float length = 100.0f);
    void                    DrawPlane(Float3 const& normal, float d, float length = 100.0f);

    void                    DrawPlaneFilled(PlaneF const& plane, float length = 100.0f, bool twoSided = false);
    void                    DrawPlaneFilled(Float3 const& normal, float d, float length = 100.0f, bool twoSided = false);

    void                    SplitCommands();
    int                     CommandsCount() const { return m_Cmds.Size(); }

    int                     GetVisPass() const { return m_VisPass; }

    DebugVertices const&    GetVertices() const { return m_Vertices; }
    DebugIndices const&     GetIndices() const { return m_Indices; }
    DebugDrawCmds const&    GetCmds() const { return m_Cmds; }

private:
    bool                    PrimitiveReserve(DBG_DRAW_CMD name, int numVertices, int numIndices, DebugDrawCmd** cmd, DebugVertex** verts, uint16_t** indices);

    Float3                  m_ViewPosition;
    DebugVertices           m_Vertices;
    DebugIndices            m_Indices;
    DebugDrawCmds           m_Cmds;
    uint32_t                m_CurrentColor{0xffffffff};
    int                     m_FirstVertex{};
    int                     m_FirstIndex{};
    int                     m_FirstDrawCommand;
    int                     m_VisPass{};
    bool                    m_DepthTest{};
    bool                    m_Split{};
    Stack<Float3x4, 4>      m_TransformStack;
    const uint32_t*         m_pColors{};
    size_t                  m_ColorMask{1};
};

HK_NAMESPACE_END
