#pragma once

#include <Hork/RenderDefs/RenderDefs.h>

HK_NAMESPACE_BEGIN

class OmnidirectionalShadowMapPool
{
public:
    OmnidirectionalShadowMapPool();

    int GetResolution() const;

    int GetSize() const;

    Ref<RenderCore::ITexture> const& GetDepthTexture() const { return DepthTexture; }

private:
    Ref<RenderCore::ITexture>   DepthTexture;
    int                         PoolSize;
};

HK_NAMESPACE_END
