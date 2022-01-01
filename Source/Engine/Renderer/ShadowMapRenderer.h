#pragma once

#include <RenderCore/FrameGraph.h>
#include <Geometry/Public/VectorMath.h>

struct SDirectionalLightInstance;
struct SLightParameters;
struct SShadowRenderInstance;

class AShadowMapRenderer {
public:
    AShadowMapRenderer();

    void AddDummyShadowMap(RenderCore::AFrameGraph& FrameGraph, RenderCore::FGTextureProxy** ppShadowMapDepth);
    void AddPass(RenderCore::AFrameGraph& FrameGraph, SDirectionalLightInstance const* Light, RenderCore::FGTextureProxy** ppShadowMapDepth);
    void AddPass(RenderCore::AFrameGraph& FrameGraph, SLightParameters const* Light, RenderCore::FGTextureProxy** ppShadowMapDepth);

private:
    void CreatePipeline();
    void CreateLightPortalPipeline();

    bool BindMaterialShadowMap( SShadowRenderInstance const * instance );

    TRef< RenderCore::IPipeline > StaticShadowCasterPipeline;
    TRef< RenderCore::IPipeline > LightPortalPipeline;
    TRef< RenderCore::ITexture > DummyShadowMap;
};

extern const Float4 EVSM_ClearValue;
extern const Float4 VSM_ClearValue;
