#pragma once

#include "RenderCommon.h"

class AShadowMapRenderer {
public:
    AShadowMapRenderer();

    void AddDummyShadowMap( AFrameGraph & FrameGraph, AFrameGraphTexture ** ppShadowMapDepth );
    void AddPass( AFrameGraph & FrameGraph, SDirectionalLightDef const * LightDef, AFrameGraphTexture ** ppShadowMapDepth );

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
