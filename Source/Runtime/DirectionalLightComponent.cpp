/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

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
#include "World.h"
#include "DebugRenderer.h"
#include <RenderCore/VertexMemoryGPU.h>

#include <Core/ConsoleVar.h>

HK_NAMESPACE_BEGIN

ConsoleVar com_DrawDirectionalLights("com_DrawDirectionalLights"s, "0"s, CVAR_CHEAT);

static constexpr int MAX_CASCADE_SPLITS = MAX_SHADOW_CASCADES + 1;

static constexpr Float4x4 ShadowMapBias = Float4x4(
    {0.5f, 0.0f, 0.0f, 0.0f},
    {0.0f, -0.5f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.5f, 0.5f, 0.0f, 1.0f});

HK_BEGIN_CLASS_META(DirectionalLightComponent)
HK_PROPERTY(IlluminanceInLux, SetIlluminance, GetIlluminance, HK_PROPERTY_DEFAULT)
HK_PROPERTY(ShadowMaxDistance, SetShadowMaxDistance, GetShadowMaxDistance, HK_PROPERTY_DEFAULT)
HK_PROPERTY(ShadowCascadeResolution, SetShadowCascadeResolution, GetShadowCascadeResolution, HK_PROPERTY_DEFAULT)
HK_PROPERTY(ShadowCascadeOffset, SetShadowCascadeOffset, GetShadowCascadeOffset, HK_PROPERTY_DEFAULT)
HK_PROPERTY(ShadowCascadeSplitLambda, SetShadowCascadeSplitLambda, GetShadowCascadeSplitLambda, HK_PROPERTY_DEFAULT)
HK_PROPERTY(MaxShadowCascades, SetMaxShadowCascades, GetMaxShadowCascades, HK_PROPERTY_DEFAULT)
HK_END_CLASS_META()

void DirectionalLightComponent::SetIlluminance(float illuminanceInLux)
{
    m_IlluminanceInLux = illuminanceInLux;
    m_bEffectiveColorDirty = true;
}

float DirectionalLightComponent::GetIlluminance() const
{
    return m_IlluminanceInLux;
}

void DirectionalLightComponent::InitializeComponent()
{
    Super::InitializeComponent();

    GetWorld()->LightingSystem.DirectionalLights.Add(this);
}

void DirectionalLightComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    GetWorld()->LightingSystem.DirectionalLights.Remove(this);
}

void DirectionalLightComponent::SetMaxShadowCascades(int maxShadowCascades)
{
    m_MaxShadowCascades = Math::Clamp(maxShadowCascades, 1, MAX_SHADOW_CASCADES);
}

int DirectionalLightComponent::GetMaxShadowCascades() const
{
    return m_MaxShadowCascades;
}

void DirectionalLightComponent::OnTransformDirty()
{
    Super::OnTransformDirty();
}

Float4 const& DirectionalLightComponent::GetEffectiveColor() const
{
    if (m_bEffectiveColorDirty)
    {
        const float EnergyUnitScale = 1.0f / 100.0f / 100.0f;

        float energy = m_IlluminanceInLux * EnergyUnitScale * GetAnimationBrightness();

        Color4 temperatureColor;
        temperatureColor.SetTemperature(GetTemperature());

        m_EffectiveColor[0] = m_Color[0] * temperatureColor[0] * energy;
        m_EffectiveColor[1] = m_Color[1] * temperatureColor[1] * energy;
        m_EffectiveColor[2] = m_Color[2] * temperatureColor[2] * energy;

        m_bEffectiveColorDirty = false;
    }
    return m_EffectiveColor;
}

void DirectionalLightComponent::DrawDebug(DebugRenderer* InRenderer)
{
    Super::DrawDebug(InRenderer);

    if (com_DrawDirectionalLights)
    {
        Float3 pos = GetWorldPosition();
        InRenderer->SetDepthTest(false);
        InRenderer->SetColor(Color4(1, 1, 1, 1));
        InRenderer->DrawLine(pos, pos + GetWorldDirection() * 10.0f);
    }
}

void DirectionalLightComponent::AddShadowmapCascades(StreamedMemoryGPU* StreamedMemory, RenderViewData* View, size_t* ViewProjStreamHandle, int* pFirstCascade, int* pNumCascades)
{
    float    cascadeSplits[MAX_CASCADE_SPLITS];
    int      numSplits = m_MaxShadowCascades + 1;
    int      numVisibleSplits;
    Float4x4 lightViewMatrix;
    Float3   worldspaceVerts[MAX_CASCADE_SPLITS][4];
    Float3   right, up;

    HK_ASSERT(m_MaxShadowCascades > 0 && m_MaxShadowCascades <= MAX_SHADOW_CASCADES);

    if (!IsCastShadow())
    {
        *pFirstCascade = 0;
        *pNumCascades  = 0;
        return;
    }

    if (View->bPerspective)
    {
        float tanFovX = std::tan(View->ViewFovX * 0.5f);
        float tanFovY = std::tan(View->ViewFovY * 0.5f);
        right         = View->ViewRightVec * tanFovX;
        up            = View->ViewUpVec * tanFovY;
    }
    else
    {
        float orthoWidth  = View->ViewOrthoMaxs.X - View->ViewOrthoMins.X;
        float orthoHeight = View->ViewOrthoMaxs.Y - View->ViewOrthoMins.Y;
        right             = View->ViewRightVec * Math::Abs(orthoWidth * 0.5f);
        up                = View->ViewUpVec * Math::Abs(orthoHeight * 0.5f);
    }

    const float shadowMaxDistance = m_ShadowMaxDistance;
    const float offset            = m_ShadowCascadeOffset;
    const float a                 = (shadowMaxDistance - offset) / View->ViewZNear;
    const float b                 = (shadowMaxDistance - offset) - View->ViewZNear;
    const float lambda            = m_ShadowCascadeSplitLambda;

    // Calc splits
    cascadeSplits[0]                      = View->ViewZNear;
    cascadeSplits[MAX_CASCADE_SPLITS - 1] = shadowMaxDistance;

    for (int splitIndex = 1; splitIndex < MAX_CASCADE_SPLITS - 1; splitIndex++)
    {
        const float factor        = (float)splitIndex / (MAX_CASCADE_SPLITS - 1);
        const float logarithmic   = View->ViewZNear * Math::Pow(a, factor);
        const float linear        = View->ViewZNear + b * factor;
        const float dist          = Math::Lerp(linear, logarithmic, lambda);
        cascadeSplits[splitIndex] = offset + dist;
    }

    float maxVisibleDist = Math::Max(View->MaxVisibleDistance, cascadeSplits[0]);

    // Calc worldspace verts
    for (numVisibleSplits = 0;
         numVisibleSplits < numSplits && (cascadeSplits[Math::Max(0, numVisibleSplits - 1)] <= maxVisibleDist);
         numVisibleSplits++)
    {
        Float3* pWorldSpaceVerts = worldspaceVerts[numVisibleSplits];

        float d = cascadeSplits[numVisibleSplits];

        // FIXME: variable distance can cause edge shimmering
        //d = d > maxVisibleDist ? maxVisibleDist : d;

        Float3 centerWorldspace = View->ViewPosition + View->ViewDir * d;

        Float3 c1 = right + up;
        Float3 c2 = right - up;

        if (View->bPerspective)
        {
            c1 *= d;
            c2 *= d;
        }

        pWorldSpaceVerts[0] = centerWorldspace - c1;
        pWorldSpaceVerts[1] = centerWorldspace - c2;
        pWorldSpaceVerts[2] = centerWorldspace + c1;
        pWorldSpaceVerts[3] = centerWorldspace + c2;
    }

    int numVisibleCascades = numVisibleSplits - 1;

    BvSphere cascadeSphere;

    Float3x3 basis     = GetWorldRotation().ToMatrix3x3().Transposed();
    lightViewMatrix[0] = Float4(basis[0], 0.0f);
    lightViewMatrix[1] = Float4(basis[1], 0.0f);
    lightViewMatrix[2] = Float4(basis[2], 0.0f);

    const float halfCascadeRes = m_ShadowCascadeResolution >> 1;
    const float oneOverHalfCascadeRes = 1.0f / halfCascadeRes;

    int firstCascade = View->NumShadowMapCascades;

    // Distance from cascade bounds to light source (near clip plane)
    // NOTE: We can calc actual light distance from scene geometry,
    // but now it just a magic number big enough to enclose most scenes = 1km.
    const float lightDistance = 1000.0f;

    Float4x4* lightViewProjectionMatrices = nullptr;
    if (numVisibleCascades > 0)
    {
        *ViewProjStreamHandle = StreamedMemory->AllocateConstant(numVisibleCascades * sizeof(Float4x4), nullptr);

        lightViewProjectionMatrices = (Float4x4*)StreamedMemory->Map(*ViewProjStreamHandle);
    }

    for (int i = 0; i < numVisibleCascades; i++)
    {
        // Calc cascade bounding sphere
        cascadeSphere.FromPointsAverage(worldspaceVerts[i], 8);

        // Set light position at cascade center
        lightViewMatrix[3] = Float4(basis * -cascadeSphere.Center, 1.0f);

        // Set ortho box
        Float3 cascadeMins = Float3(-cascadeSphere.Radius);
        Float3 cascadeMaxs = Float3(cascadeSphere.Radius);

        // Offset near clip distance
        cascadeMins[2] -= lightDistance;

        // Calc light view projection matrix
        Float4x4 cascadeMatrix = Float4x4::OrthoCC(Float2(cascadeMins), Float2(cascadeMaxs), cascadeMins[2], cascadeMaxs[2]) * lightViewMatrix;

#if 0
        // Calc pixel fraction in texture space
        Float2 error = Float2( cascadeMatrix[3] );  // same cascadeMatrix * Float4(0,0,0,1)
        error.X = Math::Fract( error.X * halfCascadeRes ) * oneOverHalfCascadeRes;
        error.Y = Math::Fract( error.Y * halfCascadeRes ) * oneOverHalfCascadeRes;

        // Snap light projection to texel grid
        // Same cascadeMatrix = Float4x4::Translation( -error ) * cascadeMatrix;
        cascadeMatrix[3].X -= error.X;
        cascadeMatrix[3].Y -= error.Y;
#else
        // Snap light projection to texel grid
        cascadeMatrix[3].X -= Math::Fract(cascadeMatrix[3].X * halfCascadeRes) * oneOverHalfCascadeRes;
        cascadeMatrix[3].Y -= Math::Fract(cascadeMatrix[3].Y * halfCascadeRes) * oneOverHalfCascadeRes;
#endif

        int cascadeIndex = firstCascade + i;

        lightViewProjectionMatrices[i]        = cascadeMatrix;
        View->ShadowMapMatrices[cascadeIndex] = ShadowMapBias * cascadeMatrix * View->ClipSpaceToWorldSpace;
    }

    View->NumShadowMapCascades += numVisibleCascades;

    *pFirstCascade = firstCascade;
    *pNumCascades  = numVisibleCascades;
}

HK_NAMESPACE_END
