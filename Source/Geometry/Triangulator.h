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

#include <Platform/Memory/LinearAllocator.h>
#include <Containers/Vector.h>
#include "VectorMath.h"

class ATriangulatorBase
{
public:
    void SetBoundary(bool _Flag);

protected:
    ATriangulatorBase();
    virtual ~ATriangulatorBase();

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
    void* tesselator_;
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
class TTriangulator : ATriangulatorBase
{
public:
    struct SPolygon
    {
        ContourVertex*                             OuterContour;
        int                                        OuterContourVertexCount;
        TVector<std::pair<ContourVertex*, int>> HoleContours;
        Double3                                    Normal;
    };

    TTriangulator(TVector<TriangleVertex>* pOutputStreamVertices, TPodVector<unsigned int>* pOutputStreamIndices);

    void Triangulate(SPolygon const* polygon);

private:
    static void OnBeginData(uint32_t topology, void* polygonData);
    static void OnEndData(void* polygonData);
    static void OnVertexData(void* data, void* polygonData);
    static void OnCombineData(double position[3], void* data[4], float weight[4], void** outData, void* polygonData);
    static bool IsTriangleValid(Double3 const& a, Double3 const& b, Double3 const& c);

    unsigned int FindOrCreateVertex(TriangleVertex* vertex);

    // Output indices pointer
    TPodVector<unsigned int>* pIndexStream_;

    // Output vertices pointer
    TVector<TriangleVertex>* pVertexStream_;

    // Offset to vertex stream
    int VertexOffset;

    // Current filling contour
    TPodVector<TriangleVertex*> primitiveIndices_;
    int                         currentTopology_;

    // Vertex cache
    TPodVector<TriangleVertex*> vertexCache_;

    // Temporary allocated verts
    TLinearAllocator<Align(sizeof(TriangleVertex), 16) * 1024> vertexAllocator_;
    TPodVector<TriangleVertex*>                                allocatedVerts_;
};

template <typename ContourVertex, typename TriangleVertex>
TTriangulator<ContourVertex, TriangleVertex>::TTriangulator(TVector<TriangleVertex>* pOutputStreamVertices, TPodVector<unsigned int>* pOutputStreamIndices) :
    pIndexStream_(pOutputStreamIndices), pVertexStream_(pOutputStreamVertices)
{
    SetCallback(CB_BEGIN_DATA, (SCallback)OnBeginData);
    SetCallback(CB_END_DATA, (SCallback)OnEndData);
    SetCallback(CB_VERTEX_DATA, (SCallback)OnVertexData);
    SetCallback(CB_COMBINE_DATA, (SCallback)OnCombineData);
}

template <typename ContourVertex, typename TriangleVertex>
void TTriangulator<ContourVertex, TriangleVertex>::OnBeginData(uint32_t topology, void* polygonData)
{
    TTriangulator<ContourVertex, TriangleVertex>* tr = static_cast<TTriangulator<ContourVertex, TriangleVertex>*>(polygonData);

    tr->primitiveIndices_.Clear();
    tr->currentTopology_ = topology;
}

template <typename ContourVertex, typename TriangleVertex>
bool TTriangulator<ContourVertex, TriangleVertex>::IsTriangleValid(Double3 const& a, Double3 const& b, Double3 const& c)
{
    double tmp1 = c.X - a.X;
    double tmp2 = b.X - a.X;
    return Math::Abs(tmp1 * (b.Y - a.Y) - tmp2 * (c.Y - a.Y)) > 0.0001 || Math::Abs(tmp1 * (b.Z - a.Z) - tmp2 * (c.Z - a.Z)) > 0.0001;
}

template <typename ContourVertex, typename TriangleVertex>
void TTriangulator<ContourVertex, TriangleVertex>::OnEndData(void* polygonData)
{
    TTriangulator<ContourVertex, TriangleVertex>* tr = static_cast<TTriangulator<ContourVertex, TriangleVertex>*>(polygonData);

    if (tr->primitiveIndices_.Size() > 2)
    {
        const int TRIANGLES      = 0x0004;
        const int TRIANGLE_STRIP = 0x0005;
        const int TRIANGLE_FAN   = 0x0006;

        int     numIndices = tr->primitiveIndices_.Size();
        Double3 v[3];

        if (tr->currentTopology_ == TRIANGLES)
        {
            for (int j = 0; j < numIndices; j += 3)
            {
                TriangulatorTraits::TriangleVertexPosition(v[0], *tr->primitiveIndices_[j + 0]);
                TriangulatorTraits::TriangleVertexPosition(v[1], *tr->primitiveIndices_[j + 1]);
                TriangulatorTraits::TriangleVertexPosition(v[2], *tr->primitiveIndices_[j + 2]);

                if (IsTriangleValid(v[0], v[1], v[2]))
                {
                    tr->pIndexStream_->Append(tr->VertexOffset + tr->FindOrCreateVertex(tr->primitiveIndices_[j + 0]));
                    tr->pIndexStream_->Append(tr->VertexOffset + tr->FindOrCreateVertex(tr->primitiveIndices_[j + 1]));
                    tr->pIndexStream_->Append(tr->VertexOffset + tr->FindOrCreateVertex(tr->primitiveIndices_[j + 2]));
                }
            }
        }
        else if (tr->currentTopology_ == TRIANGLE_FAN)
        {
            unsigned int ind[3] = {std::numeric_limits<unsigned int>::max(), 0, 0};

            TriangulatorTraits::TriangleVertexPosition(v[0], *tr->primitiveIndices_[0]);
            TriangulatorTraits::TriangleVertexPosition(v[1], *tr->primitiveIndices_[1]);

            for (int j = 0; j < numIndices - 2; j++)
            {
                TriangulatorTraits::TriangleVertexPosition(v[2], *tr->primitiveIndices_[j + 2]);

                if (IsTriangleValid(v[0], v[1], v[2]))
                {
                    if (ind[0] == std::numeric_limits<unsigned int>::max())
                    {
                        ind[0] = tr->VertexOffset + tr->FindOrCreateVertex(tr->primitiveIndices_[0]);
                    }

                    ind[1] = tr->VertexOffset + tr->FindOrCreateVertex(tr->primitiveIndices_[j + 1]);
                    ind[2] = tr->VertexOffset + tr->FindOrCreateVertex(tr->primitiveIndices_[j + 2]);

                    tr->pIndexStream_->Append(ind[0]);
                    tr->pIndexStream_->Append(ind[1]);
                    tr->pIndexStream_->Append(ind[2]);
                }

                v[1] = v[2];
            }
        }
        else if (tr->currentTopology_ == TRIANGLE_STRIP)
        {
            TriangleVertex* vertex[3];
            for (int j = 0; j < numIndices - 2; j++)
            {
                vertex[0] = tr->primitiveIndices_[j + (j & 1)];
                vertex[1] = tr->primitiveIndices_[j - (j & 1) + 1];
                vertex[2] = tr->primitiveIndices_[j + 2];

                TriangulatorTraits::TriangleVertexPosition(v[0], *vertex[0]);
                TriangulatorTraits::TriangleVertexPosition(v[1], *vertex[1]);
                TriangulatorTraits::TriangleVertexPosition(v[2], *vertex[2]);

                if (IsTriangleValid(v[0], v[1], v[2]))
                {
                    tr->pIndexStream_->Append(tr->VertexOffset + tr->FindOrCreateVertex(vertex[0]));
                    tr->pIndexStream_->Append(tr->VertexOffset + tr->FindOrCreateVertex(vertex[1]));
                    tr->pIndexStream_->Append(tr->VertexOffset + tr->FindOrCreateVertex(vertex[2]));
                }
            }
        }

        tr->primitiveIndices_.Clear();
    }
}

template <typename ContourVertex, typename TriangleVertex>
unsigned int TTriangulator<ContourVertex, TriangleVertex>::FindOrCreateVertex(TriangleVertex* vertex)
{
    int verticesCount = vertexCache_.Size();
    for (int i = 0; i < verticesCount; i++)
    {
        // Compare pointers
        if (vertexCache_[i] == vertex)
        {
            return i;
        }
    }
    vertexCache_.Append(vertex);
    return vertexCache_.Size() - 1;
}

template <typename ContourVertex, typename TriangleVertex>
void TTriangulator<ContourVertex, TriangleVertex>::OnVertexData(void* data, void* polygonData)
{
    TTriangulator<ContourVertex, TriangleVertex>* tr = static_cast<TTriangulator<ContourVertex, TriangleVertex>*>(polygonData);

    tr->primitiveIndices_.Append(static_cast<TriangleVertex*>(data) /*tr->VertexOffset + tr->FindOrCreateVertex( static_cast< TriangleVertex * >( data ) )*/);
}

template <typename ContourVertex, typename TriangleVertex>
void TTriangulator<ContourVertex, TriangleVertex>::OnCombineData(double position[3], void* data[4], float weight[4], void** outData, void* polygonData)
{
    TTriangulator<ContourVertex, TriangleVertex>* tr = static_cast<TTriangulator<ContourVertex, TriangleVertex>*>(polygonData);

    TriangleVertex* v = new (tr->vertexAllocator_.Allocate(sizeof(TriangleVertex))) TriangleVertex;

    TriangulatorTraits::CombineVertex<TriangleVertex>(
        *v,
        *((const Double3*)&position[0]),
        const_cast<const float*>(weight),
        *static_cast<TriangleVertex*>(data[0]),
        *static_cast<TriangleVertex*>(data[1]),
        *static_cast<TriangleVertex*>(data[2]),
        *static_cast<TriangleVertex*>(data[3]));

    *outData = v;
    tr->vertexCache_.Append(v);
    tr->allocatedVerts_.Append(v);
}

#ifdef HK_COMPILER_MSVC
#    pragma warning(push)
#    pragma warning(disable : 4189)
#endif

template <typename ContourVertex, typename TriangleVertex>
void TTriangulator<ContourVertex, TriangleVertex>::Triangulate(SPolygon const* polygon)
{
    Double3 tmpPosition;

    VertexOffset = pVertexStream_->Size();

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
    pVertexStream_->Resize(VertexOffset + vertexCache_.Size());
    TriangleVertex* pVertex = pVertexStream_->ToPtr() + VertexOffset;
    for (TriangleVertex const* v : vertexCache_)
    {
        TriangulatorTraits::CopyVertex(*pVertex, *v);
        pVertex++;
    }

    // Call dtor for allocated vertices
    for (TriangleVertex* v : allocatedVerts_)
    {
        v->~TriangleVertex();
    }

    vertexCache_.Clear();
    allocatedVerts_.Clear();
    vertexAllocator_.Free();
}

#ifdef HK_COMPILER_MSVC
#    pragma warning(pop)
#endif
