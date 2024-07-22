#include "OmnidirectionalShadowMapPool.h"
#include "RenderLocal.h"

HK_NAMESPACE_BEGIN

using namespace RenderCore;

ConsoleVar r_OmniShadowmapBits("r_OmniShadowmapBits"s, "16"s); // Allowed 16 or 32 bits

OmnidirectionalShadowMapPool::OmnidirectionalShadowMapPool()
{
    TEXTURE_FORMAT depthFormat;
    if (r_OmniShadowmapBits.GetInteger() <= 16)
    {
        depthFormat = TEXTURE_FORMAT_D16;
    }
    else
    {
        depthFormat = TEXTURE_FORMAT_D32;
    }

    int faceResolution = OMNISHADOW_RESOLUTION;

    PoolSize = 256;

    GDevice->CreateTexture(
        TextureDesc()
            .SetFormat(depthFormat)
            .SetResolution(TextureResolutionCubemapArray(faceResolution, PoolSize))
            .SetBindFlags(BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL),
        &DepthTexture);
}

int OmnidirectionalShadowMapPool::GetResolution() const
{
    return DepthTexture->GetWidth();
}

int OmnidirectionalShadowMapPool::GetSize() const
{
    return PoolSize;
}

HK_NAMESPACE_END
