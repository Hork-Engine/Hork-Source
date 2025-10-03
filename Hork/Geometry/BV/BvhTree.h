/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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
#include <Hork/Core/Containers/ArrayView.h>

HK_NAMESPACE_BEGIN

/**

BvhNode

*/
struct BvhNode
{
    BvAxisAlignedBox        Bounds;
    int32_t                 Index; // First primitive in leaf (Index >= 0), next node index (Index < 0)
    int32_t                 PrimitiveCount;

    bool                    IsLeaf() const;

    void                    Read(IBinaryStreamReadInterface& stream);
    void                    Write(IBinaryStreamWriteInterface& stream) const;
};

/**

BvhTree

Binary AABB-based BVH tree

*/
class BvhTree final : public Noncopyable
{
public:
                            BvhTree();

                            BvhTree(BvhTree&& rhs) noexcept;
                            BvhTree& operator=(BvhTree&& rhs) noexcept;

                            template <typename VertexType>
                            BvhTree(ArrayView<VertexType> vertices, ArrayView<unsigned int> indices, int baseVertex, unsigned int primitivesPerLeaf);

    int                     MarkRayOverlappingLeafs(Float3 const& rayStart, Float3 const& rayEnd, unsigned int* markLeafs, int maxLeafs) const;

    int                     MarkBoxOverlappingLeafs(BvAxisAlignedBox const& bounds, unsigned int* markLeafs, int maxLeafs) const;

    Vector<BvhNode> const&  GetNodes() const { return m_Nodes; }

    unsigned int const*     GetIndirection() const { return m_Indirection.ToPtr(); }

    BvAxisAlignedBox const& GetBoundingBox() const { return m_BoundingBox; }

    void                    Read(IBinaryStreamReadInterface& stream);
    void                    Write(IBinaryStreamWriteInterface& stream) const;

private:
                            BvhTree(Float3 const* vertices, size_t numVertices, size_t vertexStride, ArrayView<unsigned int> indices, int baseVertex, unsigned int primitivesPerLeaf);

    void                    Subdivide(struct BvhBuildContext& build, int axis, int firstPrimitive, int lastPrimitive, unsigned int primitivesPerLeaf, int& primitiveIndex);

    Vector<BvhNode>         m_Nodes;
    Vector<unsigned int>    m_Indirection;
    BvAxisAlignedBox        m_BoundingBox;
};

HK_FORCEINLINE BvhTree::BvhTree(BvhTree&& rhs) noexcept :
    m_Nodes(std::move(rhs.m_Nodes)),
    m_Indirection(std::move(rhs.m_Indirection)),
    m_BoundingBox(rhs.m_BoundingBox)
{
    rhs.m_BoundingBox.Clear();
}

HK_FORCEINLINE BvhTree& BvhTree::operator=(BvhTree&& rhs) noexcept
{
    m_Nodes = std::move(rhs.m_Nodes);
    m_Indirection = std::move(rhs.m_Indirection);
    m_BoundingBox = rhs.m_BoundingBox;
    rhs.m_BoundingBox.Clear();
    return *this;
}

template <typename VertexType>
HK_FORCEINLINE BvhTree::BvhTree(ArrayView<VertexType> vertices, ArrayView<unsigned int> indices, int baseVertex, unsigned int primitivesPerLeaf) :
    BvhTree(&vertices[0].Position, vertices.Size(), sizeof(VertexType), indices, baseVertex, primitivesPerLeaf)
{}

HK_FORCEINLINE bool BvhNode::IsLeaf() const
{
    return Index >= 0;
}

HK_INLINE void BvhNode::Read(IBinaryStreamReadInterface& stream)
{
    stream.ReadObject(Bounds);
    Index          = stream.ReadInt32();
    PrimitiveCount = stream.ReadInt32();
}

HK_INLINE void BvhNode::Write(IBinaryStreamWriteInterface& stream) const
{
    stream.WriteObject(Bounds);
    stream.WriteInt32(Index);
    stream.WriteInt32(PrimitiveCount);
}

HK_NAMESPACE_END
