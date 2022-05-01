#include "OmnidirectionalShadowMapPool.h"
#include "RenderLocal.h"

using namespace RenderCore;

AConsoleVar r_OminShadowmapBits(_CTS("r_OminShadowmapBits"), _CTS("16")); // Allowed 16, 24 or 32 bits
AConsoleVar r_OminShadowmapResolution(_CTS("r_OminShadowmapResolution"), _CTS("1024"));

AOmnidirectionalShadowMapPool::AOmnidirectionalShadowMapPool()
{
    RenderCore::TEXTURE_FORMAT depthFormat;
    if (r_OminShadowmapBits.GetInteger() <= 16)
    {
        depthFormat = TEXTURE_FORMAT_DEPTH16;
    }
    else if (r_OminShadowmapBits.GetInteger() <= 24)
    {
        depthFormat = TEXTURE_FORMAT_DEPTH24;
    }
    else
    {
        depthFormat = TEXTURE_FORMAT_DEPTH32;
    }

    int faceResolution = Math::ToClosestPowerOfTwo(r_OminShadowmapResolution.GetInteger());

    PoolSize = 256;

    GDevice->CreateTexture(
        STextureDesc()
            .SetFormat(depthFormat)
            .SetResolution(STextureResolutionCubemapArray(faceResolution, PoolSize))
            .SetBindFlags(BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL),
        &Texture);
}

int AOmnidirectionalShadowMapPool::GetResolution() const
{
    return Texture->GetWidth();
}

int AOmnidirectionalShadowMapPool::GetSize() const
{
    return PoolSize;
}
