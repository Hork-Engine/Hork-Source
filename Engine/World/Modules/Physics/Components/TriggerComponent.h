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

#pragma once

#include <Engine/World/Modules/Physics/PhysicsInterface.h>

#include "BodyComponent.h"

namespace JPH
{
    class Shape;
}

HK_NAMESPACE_BEGIN

class TriggerComponent final : public BodyComponent
{
    friend class PhysicsInterface;

public:
    //
    // Meta info
    //

    static constexpr ComponentMode Mode = ComponentMode::Static;

    //
    // Initial properties
    //

    /// The collision layer this body belongs to (determines if two objects can collide)
    uint8_t                 CollisionLayer = 0;

    void                    BeginPlay();
    void                    EndPlay();

private:
    PhysBodyID              m_BodyID;
    class BodyUserData*     m_UserData = nullptr;
    JPH::Shape*             m_Shape = nullptr;
};

namespace ComponentMeta
{
    template <>
    constexpr ObjectStorageType ComponentStorageType<TriggerComponent>()
    {
        return ObjectStorageType::Sparse;
    }
}

HK_NAMESPACE_END
