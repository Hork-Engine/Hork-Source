#pragma once

#include <RenderCore/FrameGraph/FrameGraph.h>
#include <Core/Public/CoreMath.h>

struct SDirectionalLightInstance;
struct SLightParameters;
struct SShadowRenderInstance;

class AShadowMapRenderer {
public:
    AShadowMapRenderer();

    void AddDummyShadowMap( AFrameGraph & FrameGraph, AFrameGraphTexture ** ppShadowMapDepth );
    void AddPass( AFrameGraph & FrameGraph, SDirectionalLightInstance const * Light, AFrameGraphTexture ** ppShadowMapDepth );
    void AddPass( AFrameGraph & FrameGraph, SLightParameters const * Light, AFrameGraphTexture ** ppShadowMapDepth );

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
