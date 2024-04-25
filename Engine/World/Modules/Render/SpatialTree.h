#pragma once

#if 0

#include <Engine/ECS/ECS.h>
#include <Engine/Geometry/BV/BvAxisAlignedBox.h>
#include <Engine/Geometry/BV/BvSphere.h>

HK_NAMESPACE_BEGIN

class SpatialArea
{
    /** Area bounding box */
    BvAxisAlignedBox Bounds; // FIXME: will be removed later?

    /** Linked portals */
    //PortalLink* PortalList;
    uint32_t FirstPortal;

    /** Movable primitives inside the area */
    //PrimitiveLink* Links;
    uint32_t FirstPrimitive;
};

class AreaPool
{
public:
    uint32_t Add()
    {
        if (!m_FreeList.IsEmpty())
        {
            int i = m_FreeList.Last();
            m_FreeList.RemoveLast();

            return i;
        }

        m_Areas.Add();
        return m_Areas.Size() - 1;
    }

    void Remove(uint32_t id)
    {
        HK_ASSERT(id < m_Areas.Size());
        HK_ASSERT(!m_Areas[id].bFree);

        m_FreeList.Add(id);
    }

private:
    TVector<SpatialArea> m_Areas;
    TVector<uint32_t> m_FreeList;
};

class SpatialTree
{
public:
    uint32_t AddArea()
    {
        return m_AreaPool.Add();
    }

    void RemoveArea(uint32_t id)
    {
        m_AreaPool.Remove(id);
    }


    // Add primitive without geometry (Geometry will be applyed in SetBounds)
    uint32_t AddPrimitive()
    {
        return 0;
    }

    // Add box primitive
    uint32_t AddPrimitive(BvAxisAlignedBox const& box)
    {
        return 0;
    }

    // Add sphere primitive
    uint32_t AddPrimitive(BvSphere const& sphere)
    {
        return 0;
    }

    void RemovePrimitive(uint32_t id)
    {}

    void AssignEntity(uint32_t primitiveId, ECS::EntityHandle entityHandle)
    {}
    //void AssignCallbacks(...) - TODO: callbacks for raycast

    // Set box geometry for the primitive
    void SetBounds(uint32_t primitiveId, BvAxisAlignedBox const& box)
    {}

    // Set sphere geometry for the primitive
    void SetBounds(uint32_t primitiveId, BvSphere const& sphere)
    {}

    //Vector<uint32_t> QueryBox(BvAxisAlignedBox const& box, SPATIAL_MASK filterMask);
    //Vector<uint32_t> QuerySphere(BvSphere const& sphere, SPATIAL_MASK filterMask);
    //Vector<uint32_t> Raycast(Float3 const& rayStart, Float3 const& rayDir, SPATIAL_MASK filterMask);
    //Vector<uint32_t> QueryFrustum(BvFrustum const& frustum, SPATIAL_MASK filterMask)

    AreaPool m_AreaPool;
};

HK_NAMESPACE_END
#endif