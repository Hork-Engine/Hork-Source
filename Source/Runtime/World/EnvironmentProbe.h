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

#pragma once

#include "SceneComponent.h"

#include <Geometry/BV/BvAxisAlignedBox.h>
#include <Geometry/BV/BvSphere.h>
#include <Geometry/BV/BvOrientedBox.h>

#include <RenderCore/Texture.h>

HK_NAMESPACE_BEGIN

class EnvironmentMap;
struct PrimitiveDef;

class EnvironmentProbe : public SceneComponent
{
    HK_COMPONENT(EnvironmentProbe, SceneComponent)

public:
    void  SetRadius(float _Radius);
    float GetRadius() const { return m_Radius; }

    void            SetEnvironmentMap(EnvironmentMap* environmentMap);
    EnvironmentMap* GetEnvironmentMap() const { return m_EnvironmentMap; }

    void SetEnabled(bool _Enabled);

    bool IsEnabled() const { return m_bEnabled; }

    BvAxisAlignedBox const& GetWorldBounds() const { return m_AABBWorldBounds; }

    Float4x4 const& GetOBBTransformInverse() const { return m_OBBTransformInverse; }

    BvSphere const& GetSphereWorldBounds() const { return m_SphereWorldBounds; }

    void PackProbe(Float4x4 const& InViewMatrix, struct ProbeParameters& Probe);

protected:
    EnvironmentProbe();
    ~EnvironmentProbe();

    void InitializeComponent() override;
    void DeinitializeComponent() override;
    void OnTransformDirty() override;
    void DrawDebug(DebugRenderer* InRenderer) override;

private:
    void UpdateWorldBounds();

    PrimitiveDef* m_Primitive;
    BvAxisAlignedBox m_AABBWorldBounds;
    Float4x4 m_OBBTransformInverse;
    BvSphere m_SphereWorldBounds;
    BvOrientedBox m_OBBWorldBounds;

    float m_Radius;
    bool m_bEnabled;

    TRef<EnvironmentMap>       m_EnvironmentMap;
    RenderCore::BindlessHandle m_IrradianceMapHandle{};
    RenderCore::BindlessHandle m_ReflectionMapHandle{};
};

HK_NAMESPACE_END
