#pragma once

#include <RenderCore/FrameGraph.h>
#include <Geometry/VectorMath.h>
#include "RenderDefs.h"

struct SDirectionalLightInstance;
struct SLightParameters;
struct SShadowRenderInstance;

class AShadowMapRenderer
{
public:
    AShadowMapRenderer();

    void AddDummyShadowMap(RenderCore::AFrameGraph& FrameGraph, RenderCore::FGTextureProxy** ppShadowMapDepth);
    void AddPass(RenderCore::AFrameGraph& FrameGraph, SDirectionalLightInstance const* Light, RenderCore::FGTextureProxy** ppShadowMapDepth);
    void AddPass(RenderCore::AFrameGraph& FrameGraph, SLightShadowmap const* ShadowMaps, int NumOmnidirectionalShadowMaps, RenderCore::FGTextureProxy** ppOmnidirectionalShadowMapArray);

private:
    void CreatePipeline();
    void CreateLightPortalPipeline();
    void CreateOmnidirectionalShadowMapPool();

    bool BindMaterialShadowMap(RenderCore::IImmediateContext* immediateCtx, SShadowRenderInstance const* instance);
    bool BindMaterialOmniShadowMap(RenderCore::IImmediateContext* immediateCtx, SShadowRenderInstance const* instance);

    TRef<RenderCore::IPipeline> StaticShadowCasterPipeline;
    TRef<RenderCore::IPipeline> LightPortalPipeline;
    TRef<RenderCore::ITexture>  DummyShadowMap;
    TRef<RenderCore::ITexture>  OmnidirectionalShadowMapArray;
};

extern const Float4 EVSM_ClearValue;
extern const Float4 VSM_ClearValue;
