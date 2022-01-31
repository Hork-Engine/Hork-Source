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

AN_CLASS_META( APunctualLightComponent )

APunctualLightComponent::APunctualLightComponent() {
    AABBWorldBounds.Clear();
    OBBTransformInverse.Clear();

    Platform::ZeroMem( &Primitive, sizeof( Primitive ) );
    Primitive.Owner = this;
    Primitive.Type = VSD_PRIMITIVE_SPHERE;
    Primitive.VisGroup = VISIBILITY_GROUP_DEFAULT;
    Primitive.QueryGroup = VSD_QUERY_MASK_VISIBLE | VSD_QUERY_MASK_VISIBLE_IN_LIGHT_PASS;
}

void APunctualLightComponent::InitializeComponent() {
    Super::InitializeComponent();

    GetLevel()->AddPrimitive( &Primitive );
}

void APunctualLightComponent::DeinitializeComponent() {
    Super::DeinitializeComponent();

    GetLevel()->RemovePrimitive( &Primitive );
}

void APunctualLightComponent::SetVisibilityGroup( int InVisibilityGroup ) {
    Primitive.VisGroup = InVisibilityGroup;
}

int APunctualLightComponent::GetVisibilityGroup() const {
    return Primitive.VisGroup;
}

void APunctualLightComponent::SetEnabled( bool _Enabled ) {
    Super::SetEnabled( _Enabled );

    if ( _Enabled ) {
        Primitive.QueryGroup |= VSD_QUERY_MASK_VISIBLE;
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_INVISIBLE;
    } else {
        Primitive.QueryGroup &= ~VSD_QUERY_MASK_VISIBLE;
        Primitive.QueryGroup |= VSD_QUERY_MASK_INVISIBLE;
    }
}
