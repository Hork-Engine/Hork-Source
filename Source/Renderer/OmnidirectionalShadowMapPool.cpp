#include "OmnidirectionalShadowMapPool.h"
#include "RenderLocal.h"

using namespace RenderCore;

AConsoleVar r_OminShadowmapBits("r_OminShadowmapBits"s, "16"s); // Allowed 16, 24 or 32 bits
AConsoleVar r_OminShadowmapResolution("r_OminShadowmapResolution"s, "1024"s);

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