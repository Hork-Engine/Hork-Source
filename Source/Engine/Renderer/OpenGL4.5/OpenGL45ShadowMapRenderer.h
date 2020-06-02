#pragma once

#include "FrameGraph.h"
#include "OpenGL45Common.h"

namespace OpenGL45 {

class AShadowMapRenderer {
public:
    AShadowMapRenderer();

    AFrameGraphTextureStorage * AddPass( AFrameGraph & FrameGraph );

private:
    void CreatePipeline();

    bool BindMaterialShadowMap( SShadowRenderInstance const * instance );

    GHI::Pipeline StaticShadowCasterPipeline;
};

extern const Float4 EVSM_ClearValue;
extern const Float4 VSM_ClearValue;

}
