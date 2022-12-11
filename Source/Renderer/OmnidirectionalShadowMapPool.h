#pragma once

#include "RenderDefs.h"

class OmnidirectionalShadowMapPool
{
public:
    OmnidirectionalShadowMapPool();

    int GetResolution() const;

    int GetSize() const;

    TRef<RenderCore::ITexture> const& GetTexture() const { return Texture; }

private:
    TRef<RenderCore::ITexture> Texture;
    int                        PoolSize;
};
