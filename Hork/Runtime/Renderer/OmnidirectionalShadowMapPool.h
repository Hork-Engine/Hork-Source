#pragma once

#include <Hork/RenderDefs/RenderDefs.h>

HK_NAMESPACE_BEGIN

class OmnidirectionalShadowMapPool
{
public:
    OmnidirectionalShadowMapPool();

    int GetResolution() const;

    int GetSize() const;

    Ref<RHI::ITexture> const& GetDepthTexture() const { return DepthTexture; }

private:
    Ref<RHI::ITexture>   DepthTexture;
    int                         PoolSize;
};

HK_NAMESPACE_END
