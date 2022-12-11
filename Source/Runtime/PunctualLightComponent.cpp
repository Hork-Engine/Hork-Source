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

#include "PunctualLightComponent.h"
#include "World.h"
#include "DebugRenderer.h"

HK_CLASS_META(PunctualLightComponent)

PunctualLightComponent::PunctualLightComponent()
{
    m_AABBWorldBounds.Clear();
    m_OBBTransformInverse.Clear();

    m_Primitive = VisibilitySystem::AllocatePrimitive();
    m_Primitive->Owner = this;
    m_Primitive->Type = VSD_PRIMITIVE_SPHERE;
    m_Primitive->VisGroup = VISIBILITY_GROUP_DEFAULT;
    m_Primitive->QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
}

PunctualLightComponent::~PunctualLightComponent()
{
    VisibilitySystem::DeallocatePrimitive(m_Primitive);
}

void PunctualLightComponent::InitializeComponent()
{
    Super::InitializeComponent();

    GetWorld()->VisibilitySystem.AddPrimitive(m_Primitive);
}

void PunctualLightComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    GetWorld()->VisibilitySystem.RemovePrimitive(m_Primitive);
}

void PunctualLightComponent::SetEnabled(bool _Enabled)
{
    Super::SetEnabled(_Enabled);

    if (_Enabled)
    {
        m_Primitive->QueryGroup |= VSD_QUERY_MASK_VISIBLE;
        m_Primitive->QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE;
    }
    else
    {
        m_Primitive->QueryGroup &= ~VSD_QUERY_MASK_VISIBLE;
        m_Primitive->QueryGroup |= VSD_QUERY_MASK_INVISIBLE;
    }
}
