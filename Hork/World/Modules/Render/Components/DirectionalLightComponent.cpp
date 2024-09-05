/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "DirectionalLightComponent.h"
#include <Hork/Core/ConsoleVar.h>
#include <Hork/World/GameObject.h>
#include <Hork/World/DebugRenderer.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawDirectionalLights("com_DrawDirectionalLights"_s, "0"_s, CVAR_CHEAT);

void DirectionalLightComponent::UpdateEffectiveColor()
{
    const float EnergyUnitScale = 1.0f / 100.0f / 100.0f;
    float energy = m_IlluminanceInLux * EnergyUnitScale;

    Color4 temperatureColor;
    temperatureColor.SetTemperature(m_Temperature);

    m_EffectiveColor[0] = m_Color[0] * temperatureColor[0] * energy;
    m_EffectiveColor[1] = m_Color[1] * temperatureColor[1] * energy;
    m_EffectiveColor[2] = m_Color[2] * temperatureColor[2] * energy;
}

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
