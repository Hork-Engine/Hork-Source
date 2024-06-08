#include "CollisionFilter.h"

HK_NAMESPACE_BEGIN

CollisionFilter::CollisionFilter()
{
    m_CollisionMask.ZeroMem();
}

void CollisionFilter::Clear()
{
    m_CollisionMask.ZeroMem();
}

void CollisionFilter::SetShouldCollide(uint32_t group1, uint32_t group2, bool shouldCollide)
{
    if (shouldCollide)
    {
        m_CollisionMask[group1] |= HK_BIT(group2);
        m_CollisionMask[group2] |= HK_BIT(group1);
    }
    else
    {
        m_CollisionMask[group1] &= ~HK_BIT(group2);
        m_CollisionMask[group2] &= ~HK_BIT(group1);
    }
}

bool CollisionFilter::ShouldCollide(uint32_t group1, uint32_t group2) const
{
    return (m_CollisionMask[group1] & HK_BIT(group2)) != 0;
}

HK_NAMESPACE_END
