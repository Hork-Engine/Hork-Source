#pragma once

#include <Engine/World/Resources/Resource_Terrain.h>
#include <Engine/World/Component.h>

HK_NAMESPACE_BEGIN

class DebugRenderer;

class TerrainComponent final : public Component
{
public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    TerrainHandle m_Resource;

    void DrawDebug(DebugRenderer& renderer);
};

HK_NAMESPACE_END
