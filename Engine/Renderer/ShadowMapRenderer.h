#pragma once

#include <Engine/RenderCore/FrameGraph.h>
#include <Engine/Math/VectorMath.h>
#include "RenderDefs.h"
#include "OmnidirectionalShadowMapPool.h"

HK_NAMESPACE_BEGIN

struct DirectionalLightInstance;
struct LightParameters;
struct ShadowRenderInstance;

class ShadowMapRenderer
{
public:
    ShadowMapRenderer();

    void AddDummyShadowMap(RenderCore::FrameGraph& FrameGraph, RenderCore::FGTextureProxy** ppShadowMapDepth);
    void AddPass(RenderCore::FrameGraph& FrameGraph, DirectionalLightInstance const* Light, RenderCore::FGTextureProxy** ppShadowMapDepth);
    void AddPass(RenderCore::FrameGraph& FrameGraph, LightShadowmap const* ShadowMaps, int NumOmnidirectionalShadowMaps, OmnidirectionalShadowMapPool& Pool, RenderCore::FGTextureProxy** ppOmnidirectionalShadowMapArray);

private:
    void CreatePipeline();
    void CreateLightPortalPipeline();

    bool BindMaterialShadowMap(RenderCore::IImmediateContext* immediateCtx, ShadowRenderInstance const* instance);
    bool BindMaterialOmniShadowMap(RenderCore::IImmediateContext* immediateCtx, ShadowRenderInstance const* instance);

    Ref<RenderCore::IPipeline> StaticShadowCasterPipeline;
    Ref<RenderCore::IPipeline> LightPortalPipeline;
    Ref<RenderCore::ITexture>  DummyShadowMap;
};

extern const Float4 EVSM_ClearValue;
extern const Float4 VSM_ClearValue;

HK_NAMESPACE_END
