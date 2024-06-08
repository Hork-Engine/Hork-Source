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

#include "BvhTree.h"
#include "BvIntersect.h"

HK_NAMESPACE_BEGIN

struct BvhPrimitiveBounds
{
    BvAxisAlignedBox Bounds;
    int              PrimitiveIndex;
};

struct BvhSplit
{
    int Axis;
    int PrimitiveIndex;
};

struct BvhBuildContext
{
    Vector<BvAxisAlignedBox>   RightBounds;
    Vector<BvhPrimitiveBounds> Primitives[3];
};

static void CalcNodeBounds(BvhPrimitiveBounds const* Primitives, int PrimCount, BvAxisAlignedBox& Bounds)
{
    HK_ASSERT(PrimCount > 0);

    BvhPrimitiveBounds const* primitive = Primitives;

    Bounds = primitive->Bounds;

    primitive++;

    for (; primitive < &Primitives[PrimCount]; primitive++)
    {
        Bounds.AddAABB(primitive->Bounds);
    }
}

static float CalcAABBVolume(BvAxisAlignedBox const& Bounds)
{
    Float3 extents = Bounds.Size();
    return extents.X * extents.Y * extents.Z;
}

static BvhSplit FindBestSplitPrimitive(BvhBuildContext& Build, int Axis, int FirstPrimitive, int PrimCount)
{
    struct CompareBoundsMax
    {
        CompareBoundsMax(const int Axis) :
            Axis(Axis) {}
        bool operator()(BvhPrimitiveBounds const& a, BvhPrimitiveBounds const& b) const
        {
            return (a.Bounds.Maxs[Axis] < b.Bounds.Maxs[Axis]);
        }
        const int Axis;
    };

    BvhPrimitiveBounds* primitives[3] = {
        Build.Primitives[0].ToPtr() + FirstPrimitive,
        Build.Primitives[1].ToPtr() + FirstPrimitive,
        Build.Primitives[2].ToPtr() + FirstPrimitive};

    for (int i = 0; i < 3; i++)
    {
        if (i != Axis)
        {
            Core::Memcpy(primitives[i], primitives[Axis], sizeof(BvhPrimitiveBounds) * PrimCount);
        }
    }

    BvAxisAlignedBox right;
    BvAxisAlignedBox left;

    BvhSplit split;
    split.Axis = -1;

    float bestSAH = Math::MaxValue<float>(); // Surface area heuristic

    const float emptyCost = 1.0f;

    for (int axis = 0; axis < 3; axis++)
    {
        BvhPrimitiveBounds* primBounds = primitives[axis];

        std::sort(primBounds, primBounds + PrimCount, CompareBoundsMax(axis));

        right.Clear();
        for (size_t i = PrimCount - 1; i > 0; i--)
        {
            right.AddAABB(primBounds[i].Bounds);
            Build.RightBounds[i - 1] = right;
        }

        left.Clear();
        for (size_t i = 1; i < PrimCount; i++)
        {
            left.AddAABB(primBounds[i - 1].Bounds);

            float sah = emptyCost + CalcAABBVolume(left) * i + CalcAABBVolume(Build.RightBounds[i - 1]) * (PrimCount - i);
            if (bestSAH > sah)
            {
                bestSAH               = sah;
                split.Axis            = axis;
                split.PrimitiveIndex  = i;
            }
        }
    }

    HK_ASSERT((split.Axis != -1) && (bestSAH < Math::MaxValue<float>()));

    return split;
}

BvhTree::BvhTree()
{
    m_BoundingBox.Clear();
}

BvhTree::BvhTree(Float3 const* Vertices, size_t NumVertices, size_t VertexStride, ArrayView<unsigned int> Indices, int BaseVertex, unsigned int PrimitivesPerLeaf)
{
    PrimitivesPerLeaf = Math::Max(PrimitivesPerLeaf, 16u);

    int indexCount = Indices.Size();
    int primCount  = indexCount / 3;

    int numLeafs = (primCount + PrimitivesPerLeaf - 1) / PrimitivesPerLeaf;

    m_Nodes.Reserve(numLeafs * 4);

    m_Indirection.ResizeInvalidate(primCount);

    BvhBuildContext build;
    build.RightBounds.ResizeInvalidate(primCount);
    build.Primitives[0].ResizeInvalidate(primCount);
    build.Primitives[1].ResizeInvalidate(primCount);
    build.Primitives[2].ResizeInvalidate(primCount);

    int primitiveIndex = 0;
    for (unsigned int i = 0; i < indexCount; i += 3, primitiveIndex++)
    {
        const size_t i0 = BaseVertex + Indices[i];
        const size_t i1 = BaseVertex + Indices[i + 1];
        const size_t i2 = BaseVertex + Indices[i + 2];

        Float3 const& v0 = *(Float3 const*)((byte const*)Vertices + i0 * VertexStride);
        Float3 const& v1 = *(Float3 const*)((byte const*)Vertices + i1 * VertexStride);
        Float3 const& v2 = *(Float3 const*)((byte const*)Vertices + i2 * VertexStride);

        BvhPrimitiveBounds& primitive = build.Primitives[0][primitiveIndex];
        primitive.PrimitiveIndex    = i; //primitiveIndex * 3; // FIXME *3

        primitive.Bounds.Mins.X = Math::Min3(v0.X, v1.X, v2.X);
        primitive.Bounds.Mins.Y = Math::Min3(v0.Y, v1.Y, v2.Y);
        primitive.Bounds.Mins.Z = Math::Min3(v0.Z, v1.Z, v2.Z);

        primitive.Bounds.Maxs.X = Math::Max3(v0.X, v1.X, v2.X);
        primitive.Bounds.Maxs.Y = Math::Max3(v0.Y, v1.Y, v2.Y);
        primitive.Bounds.Maxs.Z = Math::Max3(v0.Z, v1.Z, v2.Z);
    }

    primitiveIndex = 0;
    Subdivide(build, 0, 0, primCount, PrimitivesPerLeaf, primitiveIndex);
    m_Nodes.ShrinkToFit();

    m_BoundingBox = m_Nodes[0].Bounds;

    //size_t sz = m_Nodes.Size() * sizeof( m_Nodes[ 0 ] )
    //    + m_Indirection.Size() * sizeof( m_Indirection[ 0 ] )
    //    + sizeof( *this );

    //size_t sz2 = m_Nodes.Reserved() * sizeof( m_Nodes[0] )
    //    + m_Indirection.Reserved() * sizeof( m_Indirection[0] )
    //    + sizeof( *this );

    //LOG( "BvhTree memory usage: {}  {}\n", sz, sz2 );
}

#if 0
BvhTree::BvhTree(ArrayView<PrimitiveDef> Primitives, unsigned int PrimitivesPerLeaf)
{
    PrimitivesPerLeaf = Math::Max(PrimitivesPerLeaf, 16u);

    int numPrimitives = Primitives.Size();
    int numLeafs      = (numPrimitives + PrimitivesPerLeaf - 1) / PrimitivesPerLeaf;

    m_Nodes.Reserve(numLeafs * 4);

    m_Indirection.ResizeInvalidate(numPrimitives);

    BvhBuildContext build;
    build.RightBounds.ResizeInvalidate(numPrimitives);
    build.Primitives[0].ResizeInvalidate(numPrimitives);
    build.Primitives[1].ResizeInvalidate(numPrimitives);
    build.Primitives[2].ResizeInvalidate(numPrimitives);

    int primitiveIndex;
    for (primitiveIndex = 0; primitiveIndex < numPrimitives; primitiveIndex++)
    {
        PrimitiveDef const* primitiveDef = &Primitives[primitiveIndex];
        BvhPrimitiveBounds&    primitive    = build.Primitives[0][primitiveIndex];

        switch (primitiveDef->Type)
        {
            case VSD_PRIMITIVE_SPHERE:
                primitive.Bounds.FromSphere(primitiveDef->Sphere.Center, primitiveDef->Sphere.Radius);
                break;
            case VSD_PRIMITIVE_BOX:
            default:
                primitive.Bounds = primitiveDef->Box;
                break;
        }

        primitive.PrimitiveIndex = primitiveIndex;
    }

    primitiveIndex = 0;
    Subdivide(build, 0, 0, numPrimitives, PrimitivesPerLeaf, primitiveIndex);
    m_Nodes.ShrinkToFit();

    m_BoundingBox = m_Nodes[0].Bounds;
}
#endif

int BvhTree::MarkBoxOverlappingLeafs(BvAxisAlignedBox const& Bounds, unsigned int* MarkLeafs, int MaxLeafs) const
{
    if (!MaxLeafs)
    {
        return 0;
    }
    int n = 0;
    for (int nodeIndex = 0; nodeIndex < m_Nodes.Size();)
    {
        BvhNode const* node = &m_Nodes[nodeIndex];

        const bool bOverlap = BvBoxOverlapBox(Bounds, node->Bounds);
        const bool bLeaf    = node->IsLeaf();

        if (bLeaf && bOverlap)
        {
            MarkLeafs[n++] = nodeIndex;
            if (n == MaxLeafs)
            {
                return n;
            }
        }
        nodeIndex += (bOverlap || bLeaf) ? 1 : (-node->Index);
    }
    return n;
}

int BvhTree::MarkRayOverlappingLeafs(Float3 const& RayStart, Float3 const& RayEnd, unsigned int* MarkLeafs, int MaxLeafs) const
{
    if (!MaxLeafs)
    {
        return 0;
    }

    Float3 rayDir = RayEnd - RayStart;
    Float3 invRayDir;

    float rayLength = rayDir.Length();

    if (rayLength < 0.0001f)
    {
        return 0;
    }

    invRayDir.X = 1.0f / rayDir.X;
    invRayDir.Y = 1.0f / rayDir.Y;
    invRayDir.Z = 1.0f / rayDir.Z;

    float hitMin, hitMax;

    int n = 0;
    for (int nodeIndex = 0; nodeIndex < m_Nodes.Size();)
    {
        BvhNode const* node = &m_Nodes[nodeIndex];

        const bool bOverlap = BvRayIntersectBox(RayStart, invRayDir, node->Bounds, hitMin, hitMax) && hitMin <= 1.0f; // rayLength;
        const bool bLeaf    = node->IsLeaf();

        if (bLeaf && bOverlap)
        {
            MarkLeafs[n++] = nodeIndex;
            if (n == MaxLeafs)
            {
                return n;
            }
        }
        nodeIndex += (bOverlap || bLeaf) ? 1 : (-node->Index);
    }

    return n;
}

void BvhTree::Read(IBinaryStreamReadInterface& Stream)
{
    Stream.ReadArray(m_Nodes);
    Stream.ReadArray(m_Indirection);
    Stream.ReadObject(m_BoundingBox);
}

void BvhTree::Write(IBinaryStreamWriteInterface& Stream) const
{
    Stream.WriteArray(m_Nodes);
    Stream.WriteArray(m_Indirection);
    Stream.WriteObject(m_BoundingBox);
}

void BvhTree::Subdivide(BvhBuildContext& Build, int Axis, int FirstPrimitive, int LastPrimitive, unsigned int PrimitivesPerLeaf, int& PrimitiveIndex)
{
    BvhPrimitiveBounds* pPrimitives = Build.Primitives[Axis].ToPtr() + FirstPrimitive;
    int primCount   = LastPrimitive - FirstPrimitive;

    int curNodeInex = m_Nodes.Size();
    m_Nodes.EmplaceBack();

    CalcNodeBounds(pPrimitives, primCount, m_Nodes[curNodeInex].Bounds);

    if (primCount <= PrimitivesPerLeaf)
    {
        // Leaf

        m_Nodes[curNodeInex].Index = PrimitiveIndex;
        m_Nodes[curNodeInex].PrimitiveCount = primCount;

        for (int i = 0; i < primCount; ++i)
        {
            m_Indirection[PrimitiveIndex + i] = pPrimitives[i].PrimitiveIndex;
        }

        PrimitiveIndex += primCount;
    }
    else
    {
        // Node
        BvhSplit split = FindBestSplitPrimitive(Build, Axis, FirstPrimitive, primCount);

        int mid = FirstPrimitive + split.PrimitiveIndex;

        Subdivide(Build, split.Axis, FirstPrimitive, mid, PrimitivesPerLeaf, PrimitiveIndex);
        Subdivide(Build, split.Axis, mid, LastPrimitive, PrimitivesPerLeaf, PrimitiveIndex);

        int nextNode = m_Nodes.Size() - curNodeInex;
        m_Nodes[curNodeInex].Index   = -nextNode;
    }
}

HK_NAMESPACE_END
