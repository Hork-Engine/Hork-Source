/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "BvAxisAlignedBox.h"
#include <Containers/ArrayView.h>

HK_NAMESPACE_BEGIN

/**

BvhNode

*/
struct BvhNode
{
    BvAxisAlignedBox Bounds;
    int32_t          Index; // First primitive in leaf (Index >= 0), next node index (Index < 0)
    int32_t          PrimitiveCount;

    bool IsLeaf() const
    {
        return Index >= 0;
    }

    void Read(IBinaryStreamReadInterface& Stream)
    {
        Stream.ReadObject(Bounds);
        Index          = Stream.ReadInt32();
        PrimitiveCount = Stream.ReadInt32();
    }

    void Write(IBinaryStreamWriteInterface& Stream) const
    {
        Stream.WriteObject(Bounds);
        Stream.WriteInt32(Index);
        Stream.WriteInt32(PrimitiveCount);
    }
};

/**

BvhTree

Binary AABB-based BVH tree

*/
class BvhTree : public Noncopyable
{
public:
    BvhTree();
    virtual ~BvhTree() = default;

    BvhTree(BvhTree&& Rhs) noexcept = default;
    BvhTree& operator=(BvhTree&& Rhs) noexcept = default;

    template <typename VertexType>
    BvhTree(TArrayView<VertexType> Vertices, TArrayView<unsigned int> Indices, int BaseVertex, unsigned int PrimitivesPerLeaf) :
        BvhTree(&Vertices[0].Position, Vertices.Size(), sizeof(VertexType), Indices, BaseVertex, PrimitivesPerLeaf)
    {}

    #if 0
    BvhTree(TArrayView<PrimitiveDef> Primitives, unsigned int PrimitivesPerLeaf);
    #endif

    int MarkRayOverlappingLeafs(Float3 const& RayStart, Float3 const& RayEnd, unsigned int* MarkLeafs, int MaxLeafs) const;

    int MarkBoxOverlappingLeafs(BvAxisAlignedBox const& Bounds, unsigned int* MarkLeafs, int MaxLeafs) const;

    TVector<BvhNode> const& GetNodes() const { return m_Nodes; }

    unsigned int const* GetIndirection() const { return m_Indirection.ToPtr(); }

    BvAxisAlignedBox const& GetBoundingBox() const { return m_BoundingBox; }

    void Read(IBinaryStreamReadInterface& Stream);
    void Write(IBinaryStreamWriteInterface& Stream) const;

private:
    BvhTree(Float3 const* Vertices, size_t NumVertices, size_t VertexStride, TArrayView<unsigned int> Indices, int BaseVertex, unsigned int PrimitivesPerLeaf);

    void Subdivide(struct BvhBuildContext& Build, int Axis, int FirstPrimitive, int LastPrimitive, unsigned int PrimitivesPerLeaf, int& PrimitiveIndex);

    TVector<BvhNode>      m_Nodes;
    TVector<unsigned int> m_Indirection;
    BvAxisAlignedBox      m_BoundingBox;
};

HK_NAMESPACE_END
