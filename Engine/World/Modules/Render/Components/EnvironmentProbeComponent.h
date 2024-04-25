#pragma once

#include <Engine/Geometry/BV/BvAxisAlignedBox.h>

HK_NAMESPACE_BEGIN

struct EnvironmentProbeComponent
{
    BvAxisAlignedBox BoundingBox; // FIXME: Or OBB?
    uint32_t m_PrimID;
    uint32_t m_ProbeIndex; // Probe index inside level
};

HK_NAMESPACE_END
