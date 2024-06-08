#pragma once

#include <Engine/Core/Containers/Array.h>

HK_NAMESPACE_BEGIN

class CollisionFilter
{
public:
                            CollisionFilter();

    void                    Clear();

    void                    SetShouldCollide(uint32_t group1, uint32_t group2, bool shouldCollide);

    bool                    ShouldCollide(uint32_t group1, uint32_t group2) const;

private:
    TArray<uint32_t, 32>    m_CollisionMask;
};

HK_NAMESPACE_END
