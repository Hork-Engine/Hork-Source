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

#include <Engine/World/Modules/Physics/PhysicsInterface.h>
#include <Engine/Geometry/BV/BvAxisAlignedBox.h>

#include "BodyComponent.h"

HK_NAMESPACE_BEGIN

class TerrainCollisionData;

class HeightFieldComponent final : public BodyComponent
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    //
    // Initial properties
    //

    /// The collision group this body belongs to (determines if two objects can collide)
    uint8_t                 CollisionLayer = 0;

    Ref<TerrainCollisionData> Data;

    // Utilites

    /// Gather geometry inside crop box. Note that some triangles may be outside the box.
    /// The crop box is specified in world space.
    void                    GatherGeometry(BvAxisAlignedBox const& cropBox, Vector<Float3>& vertices, Vector<uint32_t>& indices);

    // Internal

    void                    BeginPlay();
    void                    EndPlay();

private:
    PhysBodyID              m_BodyID;
    class BodyUserData*     m_UserData = nullptr;
};

namespace ComponentMeta
{
    template <>
    constexpr ObjectStorageType ComponentStorageType<HeightFieldComponent>()
    {
        return ObjectStorageType::Sparse;
    }
}

class TerrainCollisionData : public RefCounted
{
public:
                                    TerrainCollisionData();

    void                            Create(const float* inSamples, uint32_t inSampleCount/*, const uint8_t* inMaterialIndices = nullptr, const JPH::PhysicsMaterialList& inMaterialList = JPH::PhysicsMaterialList()*/);

    /// Get height field position at sampled location (inX, inY).
    /// where inX and inY are integers in the range inX e [0, mSampleCount - 1] and inY e [0, mSampleCount - 1].
    Float3                          GetPosition(uint32_t inX, uint32_t inY) const;

    /// Check if height field at sampled location (inX, inY) has collision (has a hole or not)
    bool                            IsNoCollision(uint32_t inX, uint32_t inY) const;

    /// Projects inLocalPosition (a point in the space of the shape) along the Y axis onto the surface and returns it in outSurfacePosition.
    /// When there is no surface position (because of a hole or because the point is outside the heightfield) the function will return false.
    bool                            ProjectOntoSurface(Float3 const& inLocalPosition, Float3& outSurfacePosition, Float3& outSurfaceNormal) const;

    /// Amount of memory used by height field (size in bytes)
    size_t                          GetMemoryUsage() const;

    void                            GatherGeometry(BvAxisAlignedBox const& inLocalBounds, Vector<Float3>& outVertices, Vector<unsigned int>& outIndices) const;

    MeshCollisionDataInternal const* GetData() { return m_Data.RawPtr(); }

private:
    UniqueRef<MeshCollisionDataInternal> m_Data;
};

HK_NAMESPACE_END
