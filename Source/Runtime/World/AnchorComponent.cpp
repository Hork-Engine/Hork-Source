/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include "AnchorComponent.h"

#include <Runtime/BulletCompatibility.h>

HK_NAMESPACE_BEGIN

HK_BEGIN_CLASS_META(AnchorComponent)
HK_END_CLASS_META()

AnchorComponent::AnchorComponent()
{
}

void AnchorComponent::InitializeComponent()
{
}

void AnchorComponent::DeinitializeComponent()
{
    Super::DeinitializeComponent();

    btCollisionShape* shape = Anchor->getCollisionShape();
    delete Anchor;
    Anchor = nullptr;
    delete shape;
    shape = nullptr;
}

void AnchorComponent::OnTransformDirty()
{
    btTransform transform;

    Float3 worldPosition = GetWorldPosition();
    Quat   worldRotation = GetWorldRotation();

    transform.setOrigin(btVectorToFloat3(worldPosition));
    transform.setRotation(btQuaternionToQuat(worldRotation));

    if (Anchor)
    {
        Anchor->setWorldTransform(transform);
    }

    //PrevWorldPosition = worldPosition;
    //PrevWorldRotation = worldRotation;

    //PrevTransformOrigin = btVectorToFloat3( transform.getOrigin() );
    //PrevTransformBasis = btMatrixToFloat3x3( transform.getBasis() );

    //bUpdateSoftbodyTransform = false;
}

HK_NAMESPACE_END
