#include "OmnidirectionalShadowMapPool.h"
#include "RenderLocal.h"

using namespace RenderCore;

ConsoleVar r_OminShadowmapBits("r_OminShadowmapBits"s, "16"s); // Allowed 16 or 32 bits
ConsoleVar r_OminShadowmapResolution("r_OminShadowmapResolution"s, "1024"s);

OmnidirectionalShadowMapPool::OmnidirectionalShadowMapPool()
{
    TEXTURE_FORMAT depthFormat;
    if (r_OminShadowmapBits.GetInteger() <= 16)
    {
        depthFormat = TEXTURE_FORMAT_D16;
    }
    else
    {
        depthFormat = TEXTURE_FORMAT_D32;
    }

    int faceResolution = Math::ToClosestPowerOfTwo(r_OminShadowmapResolution.GetInteger());

    PoolSize = 256;

    GDevice->CreateTexture(
        TextureDesc()
            .SetFormat(depthFormat)
            .SetResolution(TextureResolutionCubemapArray(faceResolution, PoolSize))
            .SetBindFlags(BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL),
        &Texture);
}

int OmnidirectionalShadowMapPool::GetResolution() const
{
    return Texture->GetWidth();
}

int OmnidirectionalShadowMapPool::GetSize() const
{
    return PoolSize;
}
