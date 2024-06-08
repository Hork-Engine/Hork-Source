#include "DirectionalLightComponent.h"
#include <Engine/Core/ConsoleVar.h>
#include <Engine/World/GameObject.h>
#include <Engine/World/DebugRenderer.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawDirectionalLights("com_DrawDirectionalLights"s, "0"s, CVAR_CHEAT);

void DirectionalLightComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawDirectionalLights)
    {
        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(1, 1, 1, 1));

        renderer.SetColor(Color4(m_EffectiveColor[0],
                                 m_EffectiveColor[1],
                                 m_EffectiveColor[2]));

        Float3 position = GetOwner()->GetWorldPosition();
        Float3 dir = GetOwner()->GetWorldDirection();
        renderer.DrawLine(position, position + dir * 10.0f);
    }
}

HK_NAMESPACE_END
