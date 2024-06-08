#pragma once

#include <Engine/Geometry/BV/BvAxisAlignedBox.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;

struct EnvironmentProbeComponent
{
    BvAxisAlignedBox BoundingBox; // FIXME: Or OBB?
    uint32_t m_PrimID;
    uint32_t m_ProbeIndex; // Probe index inside level

    void DrawDebug(DebugRenderer& renderer);
};

HK_NAMESPACE_END
