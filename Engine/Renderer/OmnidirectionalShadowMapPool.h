#pragma once

#include "RenderDefs.h"

HK_NAMESPACE_BEGIN

class OmnidirectionalShadowMapPool
{
public:
    OmnidirectionalShadowMapPool();

    int GetResolution() const;

    int GetSize() const;

    Ref<RenderCore::ITexture> const& GetTexture() const { return Texture; }

private:
    Ref<RenderCore::ITexture> Texture;
    int                        PoolSize;
};

HK_NAMESPACE_END
