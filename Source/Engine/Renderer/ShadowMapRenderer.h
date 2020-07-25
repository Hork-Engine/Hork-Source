#pragma once

#include "RenderCommon.h"

class AShadowMapRenderer {
public:
    AShadowMapRenderer();

    AFrameGraphTexture * AddPass( AFrameGraph & FrameGraph );

private:
    void CreatePipeline();
    void CreateLightPortalPipeline();

    bool BindMaterialShadowMap( SShadowRenderInstance const * instance );

    TRef< RenderCore::IPipeline > StaticShadowCasterPipeline;
    TRef< RenderCore::IPipeline > LightPortalPipeline;
};

extern const Float4 EVSM_ClearValue;
extern const Float4 VSM_ClearValue;
