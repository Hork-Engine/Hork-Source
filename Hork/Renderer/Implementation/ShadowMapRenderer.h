#pragma once

#include <Hork/RHI/Common/FrameGraph.h>
#include <Hork/Math/VectorMath.h>
#include <Hork/Renderer/RenderDefs.h>
#include "OmnidirectionalShadowMapPool.h"

HK_NAMESPACE_BEGIN

struct DirectionalLightInstance;
struct LightParameters;
struct ShadowRenderInstance;

class ShadowMapRenderer
{
public:
    ShadowMapRenderer();

    void AddDummyShadowMap(RHI::FrameGraph& FrameGraph, RHI::FGTextureProxy** ppShadowMapDepth);
    void AddPass(RHI::FrameGraph& FrameGraph, DirectionalLightInstance const* Light, RHI::FGTextureProxy** ppShadowMapDepth);
    void AddPass(RHI::FrameGraph& FrameGraph, LightShadowmap const* ShadowMaps, int NumOmnidirectionalShadowMaps, OmnidirectionalShadowMapPool& Pool, RHI::FGTextureProxy** ppOmnidirectionalShadowMapArray);

private:
    void CreatePipeline();
    void CreateLightPortalPipeline();

    bool BindMaterialShadowMap(RHI::IImmediateContext* immediateCtx, ShadowRenderInstance const* instance);
    bool BindMaterialOmniShadowMap(RHI::IImmediateContext* immediateCtx, ShadowRenderInstance const* instance);

    Ref<RHI::IPipeline> StaticShadowCasterPipeline;
    Ref<RHI::IPipeline> LightPortalPipeline;
    Ref<RHI::ITexture>  DummyShadowMap;
};

extern const Float4 EVSM_ClearValue;
extern const Float4 VSM_ClearValue;

HK_NAMESPACE_END
