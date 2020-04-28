#pragma once

#include "OpenGL45PassRenderer.h"

namespace OpenGL45 {

class AShadowMapPassRenderer : public APassRenderer {
public:
    void Initialize();
    void Deinitialize();

    GHI::RenderPass * GetRenderPass() { return &DepthPass; }

    void Render();

    GHI::Sampler GetShadowBlockerSampler() { return ShadowDepthSampler0; }
    GHI::Sampler GetShadowDepthSampler() { return ShadowDepthSampler1; }

private:
    void CreateShadowDepthSamplers();
    void CreateRenderPass();
    void CreatePipeline();
    bool BindMaterial( SShadowRenderInstance const * instance );
    void BindTexturesShadowMapPass( SMaterialFrameData * _Instance );

    GHI::Sampler ShadowDepthSampler0;
    GHI::Sampler ShadowDepthSampler1;
    GHI::RenderPass DepthPass;
    GHI::Pipeline StaticShadowCasterPipeline;
};

extern AShadowMapPassRenderer GShadowMapPassRenderer;

}
