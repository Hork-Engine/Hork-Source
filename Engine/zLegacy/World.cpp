
#if 0

/** World. Defines a game map or editor/tool scene */
class World
{
public:
    void SetGlobalEnvironmentMap(EnvironmentMap* EnvironmentMap);
    EnvironmentMap* GetGlobalEnvironmentMap() const { return m_GlobalEnvironmentMap; }

    Ref<EnvironmentMap> m_GlobalEnvironmentMap;
};

bool World::Raycast(WorldRaycastResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastTriangles(Result, RayStart, RayEnd, Filter);
}

bool World::RaycastBounds(Vector<BoxHitResult>& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastBounds(Result, RayStart, RayEnd, Filter);
}

bool World::RaycastClosest(WorldRaycastClosestResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastClosest(Result, RayStart, RayEnd, Filter);
}

bool World::RaycastClosestBounds(BoxHitResult& Result, Float3 const& RayStart, Float3 const& RayEnd, WorldRaycastFilter const* Filter) const
{
    return VisibilitySystem.RaycastClosestBounds(Result, RayStart, RayEnd, Filter);
}

void World::QueryVisiblePrimitives(Vector<PrimitiveDef*>& VisPrimitives, Vector<SurfaceDef*>& VisSurfs, int* VisPass, VisibilityQuery const& Query)
{
    VisibilitySystem.QueryVisiblePrimitives(VisPrimitives, VisSurfs, VisPass, Query);
}

void World::QueryOverplapAreas(BvAxisAlignedBox const& Bounds, Vector<VisArea*>& Areas)
{
    VisibilitySystem.QueryOverplapAreas(Bounds, Areas);
}

void World::QueryOverplapAreas(BvSphere const& Bounds, Vector<VisArea*>& Areas)
{
    VisibilitySystem.QueryOverplapAreas(Bounds, Areas);
}

void World::SetGlobalEnvironmentMap(EnvironmentMap* EnvironmentMap)
{
    m_GlobalEnvironmentMap = EnvironmentMap;
}

#endif
