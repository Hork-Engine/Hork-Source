#include <Engine/GameEngine/Public/DrawSurf.h>
//#include <Engine/GameEngine/Public/SkeletalAnimation.h>
//#include <Engine/GameEngine/Public/World.h>
//#include <Engine/Core/Public/IntrusiveLinkedListMacro.h>

AN_CLASS_META_NO_ATTRIBS( FDrawSurf )

//AN_SCENE_COMPONENT_BEGIN_DECL( FDrawSurf, CCF_HIDDEN_IN_EDITOR )
//AN_ATTRIBUTE( "Rendering Layers", FProperty( FDrawSurf::RENDERING_LAYERS_DEFAULT ), SetRenderingLayers, GetRenderingLayers, "Set rendering layer(s)", AF_BITFIELD )
//AN_SCENE_COMPONENT_END_DECL

FDrawSurf::FDrawSurf() {
    RenderingGroup = RENDERING_GROUP_DEFAULT;
}
