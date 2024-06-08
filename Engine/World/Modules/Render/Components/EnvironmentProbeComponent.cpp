#include "EnvironmentProbeComponent.h"
#include <Engine/Core/ConsoleVar.h>
#include <Engine/World/DebugRenderer.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawEnvironmentProbes("com_DrawEnvironmentProbes"s, "0"s, CVAR_CHEAT);

void EnvironmentProbeComponent::DrawDebug(DebugRenderer& renderer)
{
    if (com_DrawEnvironmentProbes)
    {
        //if (m_Primitive->VisPass != renderer.GetVisPass()) // TODO
        //    continue;
        renderer.SetDepthTest(false);
        renderer.SetColor(Color4(1, 0, 1, 1));
        renderer.DrawAABB(BoundingBox);
    }
}

HK_NAMESPACE_END
