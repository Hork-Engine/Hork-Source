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

#include <Engine/Core/Allocators/LinearAllocator.h>
#include <Engine/Core/Containers/Vector.h>
#include <Engine/Math/VectorMath.h>

HK_NAMESPACE_BEGIN

class TriangulatorBase
{
public:
    void SetBoundary(bool _Flag);

protected:
    TriangulatorBase();
    virtual ~TriangulatorBase();

    typedef void (*SCallback)();

    void SetNormal(Double3 const& normal);
    void SetCallback(uint32_t name, SCallback callback);

    void BeginPolygon(void* data);
    void EndPolygon();
    void BeginContour();
    void EndContour();
    void ProcessVertex(Double3& vertex, const void* data);

    enum
    {
        CB_BEGIN_DATA   = 100106,
        CB_END_DATA     = 100108,
        CB_VERTEX_DATA  = 100107,
        CB_COMBINE_DATA = 100111
    };

private:
    // Tesselator handle
    void* m_Tesselator;
};

namespace TriangulatorTraits
{

template <typename ContourVertex>
void ContourVertexPosition(Double3& dst, ContourVertex const& src);

template <typename TriangleVertex>
void TriangleVertexPosition(Double3& dst, TriangleVertex const& src);

template <typename TriangleVertex>
void CombineVertex(TriangleVertex&       outputVertex,
                   Double3 const&        position,
                   float const*          weights,
                   TriangleVertex const& v0,
                   TriangleVertex const& v1,
                   TriangleVertex const& v2,
                   TriangleVertex const& v3);

template <typename ContourVertex, typename TriangleVertex>
void CopyVertex(TriangleVertex& dst, ContourVertex const& src);

} // namespace TriangulatorTraits

template <typename ContourVertex, typename TriangleVertex>
class Triangulator : TriangulatorBase
{
public:
    struct Polygon
    {
        ContourVertex*                          OuterContour;
        int                                     OuterContourVertexCount;
        Vector<std::pair<ContourVertex*, int>> HoleContours;
        Double3                                 Normal;
    };

    Triangulator(Vector<TriangleVertex>* pOutputStreamVertices, Vector<unsigned int>* pOutputStreamIndices);

    void Triangulate(Polygon const* polygon);

private:
    static void OnBeginData(uint32_t topology, void* polygonData);
    static void OnEndData(void* polygonData);
    static void OnVertexData(void* data, void* polygonData);
    static void OnCombineData(double position[3], void* data[4], float weight[4], void** outData, void* polygonData);
    static bool IsTriangleValid(Double3 const& a, Double3 const& b, Double3 const& c);

    unsigned int FindOrCreateVertex(TriangleVertex* vertex);

    // Output indices pointer
    Vector<unsigned int>* m_pIndexStream;

    // Output vertices pointer
    Vector<TriangleVertex>* m_pVertexStream;

    // Offset to vertex stream
    int m_VertexOffset;

    // Current filling contour
    Vector<TriangleVertex*> m_PrimitiveIndices;
    int                     m_CurrentTopology;

    // Vertex cache
    Vector<TriangleVertex*> m_VertexCache;

    // Temporary allocated verts
    LinearAllocator<Align(sizeof(TriangleVertex), 16) * 1024> m_VertexAllocator;
    Vector<TriangleVertex*>                                   m_AllocatedVerts;
};

template <typename ContourVertex, typename TriangleVertex>
Triangulator<ContourVertex, TriangleVertex>::Triangulator(Vector<TriangleVertex>* pOutputStreamVertices, Vector<unsigned int>* pOutputStreamIndices) :
    m_pIndexStream(pOutputStreamIndices), m_pVertexStream(pOutputStreamVertices)
{
    SetCallback(CB_BEGIN_DATA, (SCallback)OnBeginData);
    SetCallback(CB_END_DATA, (SCallback)OnEndData);
    SetCallback(CB_VERTEX_DATA, (SCallback)OnVertexData);
    SetCallback(CB_COMBINE_DATA, (SCallback)OnCombineData);
}

template <typename ContourVertex, typename TriangleVertex>
void Triangulator<ContourVertex, TriangleVertex>::OnBeginData(uint32_t topology, void* polygonData)
{
    Triangulator<ContourVertex, TriangleVertex>* tr = static_cast<Triangulator<ContourVertex, TriangleVertex>*>(polygonData);

    tr->m_PrimitiveIndices.Clear();
    tr->m_CurrentTopology = topology;
}

template <typename ContourVertex, typename TriangleVertex>
bool Triangulator<ContourVertex, TriangleVertex>::IsTriangleValid(Double3 const& a, Double3 const& b, Double3 const& c)
{
    double tmp1 = c.X - a.X;
    double tmp2 = b.X - a.X;
    return Math::Abs(tmp1 * (b.Y - a.Y) - tmp2 * (c.Y - a.Y)) > 0.0001 || Math::Abs(tmp1 * (b.Z - a.Z) - tmp2 * (c.Z - a.Z)) > 0.0001;
}

template <typename ContourVertex, typename TriangleVertex>
void Triangulator<ContourVertex, TriangleVertex>::OnEndData(void* polygonData)
{
    Triangulator<ContourVertex, TriangleVertex>* tr = static_cast<Triangulator<ContourVertex, TriangleVertex>*>(polygonData);

    if (tr->m_PrimitiveIndices.Size() > 2)
    {
        const int TRIANGLES      = 0x0004;
        const int TRIANGLE_STRIP = 0x0005;
        const int TRIANGLE_FAN   = 0x0006;

        int     numIndices = tr->m_PrimitiveIndices.Size();
        Double3 v[3];

        if (tr->m_CurrentTopology == TRIANGLES)
        {
            for (int j = 0; j < numIndices; j += 3)
            {
                TriangulatorTraits::TriangleVertexPosition(v[0], *tr->m_PrimitiveIndices[j + 0]);
                TriangulatorTraits::TriangleVertexPosition(v[1], *tr->m_PrimitiveIndices[j + 1]);
                TriangulatorTraits::TriangleVertexPosition(v[2], *tr->m_PrimitiveIndices[j + 2]);

                if (IsTriangleValid(v[0], v[1], v[2]))
                {
                    tr->m_pIndexStream->Add(tr->m_VertexOffset + tr->FindOrCreateVertex(tr->m_PrimitiveIndices[j + 0]));
                    tr->m_pIndexStream->Add(tr->m_VertexOffset + tr->FindOrCreateVertex(tr->m_PrimitiveIndices[j + 1]));
                    tr->m_pIndexStream->Add(tr->m_VertexOffset + tr->FindOrCreateVertex(tr->m_PrimitiveIndices[j + 2]));
                }
            }
        }
        else if (tr->m_CurrentTopology == TRIANGLE_FAN)
        {
            unsigned int ind[3] = {std::numeric_limits<unsigned int>::max(), 0, 0};

            TriangulatorTraits::TriangleVertexPosition(v[0], *tr->m_PrimitiveIndices[0]);
            TriangulatorTraits::TriangleVertexPosition(v[1], *tr->m_PrimitiveIndices[1]);

            for (int j = 0; j < numIndices - 2; j++)
            {
                TriangulatorTraits::TriangleVertexPosition(v[2], *tr->m_PrimitiveIndices[j + 2]);

                if (IsTriangleValid(v[0], v[1], v[2]))
                {
                    if (ind[0] == std::numeric_limits<unsigned int>::max())
                    {
                        ind[0] = tr->m_VertexOffset + tr->FindOrCreateVertex(tr->m_PrimitiveIndices[0]);
                    }

                    ind[1] = tr->m_VertexOffset + tr->FindOrCreateVertex(tr->m_PrimitiveIndices[j + 1]);
                    ind[2] = tr->m_VertexOffset + tr->FindOrCreateVertex(tr->m_PrimitiveIndices[j + 2]);

                    tr->m_pIndexStream->Add(ind[0]);
                    tr->m_pIndexStream->Add(ind[1]);
                    tr->m_pIndexStream->Add(ind[2]);
                }

                v[1] = v[2];
            }
        }
        else if (tr->m_CurrentTopology == TRIANGLE_STRIP)
        {
            TriangleVertex* vertex[3];
            for (int j = 0; j < numIndices - 2; j++)
            {
                vertex[0] = tr->m_PrimitiveIndices[j + (j & 1)];
                vertex[1] = tr->m_PrimitiveIndices[j - (j & 1) + 1];
                vertex[2] = tr->m_PrimitiveIndices[j + 2];

                TriangulatorTraits::TriangleVertexPosition(v[0], *vertex[0]);
                TriangulatorTraits::TriangleVertexPosition(v[1], *vertex[1]);
                TriangulatorTraits::TriangleVertexPosition(v[2], *vertex[2]);

                if (IsTriangleValid(v[0], v[1], v[2]))
                {
                    tr->m_pIndexStream->Add(tr->m_VertexOffset + tr->FindOrCreateVertex(vertex[0]));
                    tr->m_pIndexStream->Add(tr->m_VertexOffset + tr->FindOrCreateVertex(vertex[1]));
                    tr->m_pIndexStream->Add(tr->m_VertexOffset + tr->FindOrCreateVertex(vertex[2]));
                }
            }
        }

        tr->m_PrimitiveIndices.Clear();
    }
}

template <typename ContourVertex, typename TriangleVertex>
unsigned int Triangulator<ContourVertex, TriangleVertex>::FindOrCreateVertex(TriangleVertex* vertex)
{
    int verticesCount = m_VertexCache.Size();
    for (int i = 0; i < verticesCount; i++)
    {
        // Compare pointers
        if (m_VertexCache[i] == vertex)
        {
            return i;
        }
    }
    m_VertexCache.Add(vertex);
    return m_VertexCache.Size() - 1;
}

template <typename ContourVertex, typename TriangleVertex>
void Triangulator<ContourVertex, TriangleVertex>::OnVertexData(void* data, void* polygonData)
{
    Triangulator<ContourVertex, TriangleVertex>* tr = static_cast<Triangulator<ContourVertex, TriangleVertex>*>(polygonData);

    tr->m_PrimitiveIndices.Add(static_cast<TriangleVertex*>(data) /*tr->m_VertexOffset + tr->FindOrCreateVertex( static_cast< TriangleVertex * >( data ) )*/);
}

template <typename ContourVertex, typename TriangleVertex>
void Triangulator<ContourVertex, TriangleVertex>::OnCombineData(double position[3], void* data[4], float weight[4], void** outData, void* polygonData)
{
    Triangulator<ContourVertex, TriangleVertex>* tr = static_cast<Triangulator<ContourVertex, TriangleVertex>*>(polygonData);

    TriangleVertex* v = new (tr->m_VertexAllocator.Allocate(sizeof(TriangleVertex))) TriangleVertex;

    TriangulatorTraits::CombineVertex<TriangleVertex>(
        *v,
        *((const Double3*)&position[0]),
        const_cast<const float*>(weight),
        *static_cast<TriangleVertex*>(data[0]),
        *static_cast<TriangleVertex*>(data[1]),
        *static_cast<TriangleVertex*>(data[2]),
        *static_cast<TriangleVertex*>(data[3]));

    *outData = v;
    tr->m_VertexCache.Add(v);
    tr->m_AllocatedVerts.Add(v);
}

#ifdef HK_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4189)
#endif

template <typename ContourVertex, typename TriangleVertex>
void Triangulator<ContourVertex, TriangleVertex>::Triangulate(Polygon const* polygon)
{
    Double3 tmpPosition;

    m_VertexOffset = m_pVertexStream->Size();

    SetNormal(polygon->Normal);

    BeginPolygon(this);

    BeginContour();
    ContourVertex const* outerContour    = polygon->OuterContour;
    ContourVertex const* outerContourEnd = polygon->OuterContour + polygon->OuterContourVertexCount;
    while (outerContour < outerContourEnd)
    {
        TriangulatorTraits::ContourVertexPosition(tmpPosition, *outerContour);
        ProcessVertex(tmpPosition, outerContour);
        outerContour++;
    }
    EndContour();

    for (int i = 0; i < polygon->HoleContours.Size(); i++)
    {
        ContourVertex const* holeContour    = polygon->HoleContours[i].first;
        ContourVertex const* holeContourEnd = polygon->HoleContours[i].first + polygon->HoleContours[i].second;

        BeginContour();
        while (holeContour < holeContourEnd)
        {
            TriangulatorTraits::ContourVertexPosition(tmpPosition, *holeContour);
            ProcessVertex(tmpPosition, holeContour);
            holeContour++;
        }
        EndContour();
    }

    EndPolygon();

    // Fill vertices
    m_pVertexStream->Resize(m_VertexOffset + m_VertexCache.Size());
    TriangleVertex* pVertex = m_pVertexStream->ToPtr() + m_VertexOffset;
    for (TriangleVertex const* v : m_VertexCache)
    {
        TriangulatorTraits::CopyVertex(*pVertex, *v);
        pVertex++;
    }

    // Call dtor for allocated vertices
    for (TriangleVertex* v : m_AllocatedVerts)
    {
        v->~TriangleVertex();
    }

    m_VertexCache.Clear();
    m_AllocatedVerts.Clear();
    m_VertexAllocator.Free();
}

#ifdef HK_COMPILER_MSVC
#    pragma warning(pop)
#endif

HK_NAMESPACE_END
