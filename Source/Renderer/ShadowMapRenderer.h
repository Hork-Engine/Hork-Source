#pragma once

#include <RenderCore/FrameGraph.h>
#include <Geometry/VectorMath.h>
#include "RenderDefs.h"
#include "OmnidirectionalShadowMapPool.h"

struct SDirectionalLightInstance;
struct SLightParameters;
struct SShadowRenderInstance;

class AShadowMapRenderer
{
public:
    AShadowMapRenderer();

    void AddDummyShadowMap(RenderCore::AFrameGraph& FrameGraph, RenderCore::FGTextureProxy** ppShadowMapDepth);
    void AddPass(RenderCore::AFrameGraph& FrameGraph, SDirectionalLightInstance const* Light, RenderCore::FGTextureProxy** ppShadowMapDepth);
    void AddPass(RenderCore::AFrameGraph& FrameGraph, SLightShadowmap const* ShadowMaps, int NumOmnidirectionalShadowMaps, AOmnidirectionalShadowMapPool& Pool, RenderCore::FGTextureProxy** ppOmnidirectionalShadowMapArray);

private:
    void CreatePipeline();
    void CreateLightPortalPipeline();

    bool BindMaterialShadowMap(RenderCore::IImmediateContext* immediateCtx, SShadowRenderInstance const* instance);
    bool BindMaterialOmniShadowMap(RenderCore::IImmediateContext* immediateCtx, SShadowRenderInstance const* instance);

    TRef<RenderCore::IPipeline> StaticShadowCasterPipeline;
    TRef<RenderCore::IPipeline> LightPortalPipeline;
    TRef<RenderCore::ITexture>  DummyShadowMap;
};

extern const Float4 EVSM_ClearValue;
extern const Float4 VSM_ClearValue;
