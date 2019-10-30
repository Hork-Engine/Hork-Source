#pragma once

#include "OpenGL45PassRenderer.h"

namespace OpenGL45 {

class FShadowMapPassRenderer : public FPassRenderer {
public:
    void Initialize();
    void Deinitialize();

    GHI::RenderPass * GetRenderPass() { return &DepthPass; }

    void RenderInstances();

    GHI::Sampler GetShadowBlockerSampler() { return ShadowDepthSampler0; }
    GHI::Sampler GetShadowDepthSampler() { return ShadowDepthSampler1; }

private:
    void CreateShadowDepthSamplers();
    void CreateRenderPass();
    bool BindMaterial( FShadowRenderInstance const * instance );
    void BindTexturesShadowMapPass( FMaterialFrameData * _Instance );

    GHI::Sampler ShadowDepthSampler0;
    GHI::Sampler ShadowDepthSampler1;
    GHI::RenderPass DepthPass;
};

extern FShadowMapPassRenderer GShadowMapPassRenderer;

}
