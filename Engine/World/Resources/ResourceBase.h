#pragma once

#include <Engine/Core/BaseTypes.h>

HK_NAMESPACE_BEGIN

enum RESOURCE_TYPE : uint8_t
{
    RESOURCE_UNDEFINED,
    RESOURCE_MESH,//ok
    RESOURCE_SKELETON,//ok
    RESOURCE_NODE_MOTION,     // todo
    RESOURCE_TEXTURE,//ok
    RESOURCE_MATERIAL,// combine compiledmaterial with material
    RESOURCE_COLLISION,// todo
    RESOURCE_SOUND,//ok
    RESOURCE_FONT,//ok
    RESOURCE_TERRAIN,// ok
    RESOURCE_VIRTUAL_TEXTURE,// todo


    // BAKE:
    //
    // Navigation Mesh
    // Lightmaps
    // Photometric profiles
    // Envmaps? - can be streamed lod by lod
    // Collision models
    // Areas and portals (spatial structure)

    RESOURCE_TYPE_MAX
};

class ResourceBase
{
public:
    virtual ~ResourceBase() = default;

    virtual void Upload() {}
};

HK_FORCEINLINE uint32_t MakeResourceMagic(uint8_t type, uint8_t version)
{
    return (uint32_t('H')) | (uint32_t('k') << 8) | (uint32_t(type) << 16) | (uint32_t(version) << 24);
}

HK_NAMESPACE_END
