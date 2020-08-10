#pragma once

#include "RenderCommon.h"

class AShadowMapRenderer {
public:
    AShadowMapRenderer();

    void AddPass( AFrameGraph & FrameGraph, AFrameGraphTexture ** ppShadowMapDepth );

private:
    void CreatePipeline();
    void CreateLightPortalPipeline();

    bool BindMaterialShadowMap( SShadowRenderInstance const * instance );

    TRef< RenderCore::IPipeline > StaticShadowCasterPipeline;
    TRef< RenderCore::IPipeline > LightPortalPipeline;
};

extern const Float4 EVSM_ClearValue;
extern const Float4 VSM_ClearValue;
