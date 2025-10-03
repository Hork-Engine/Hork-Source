/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2025 Alexander Samusev.

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

#include <Hork/Runtime/World/Modules/Physics/CollisionFilter.h>

using namespace Hk;

class CollisionLayer
{
public:
    enum : uint8_t
    {
        Default,
        Character,
        CharacterOnlyTrigger,
        Bullets,
        Teleporter,
    };

    static CollisionFilter CreateFilter()
    {
        CollisionFilter collisionFilter;
        collisionFilter.SetShouldCollide(CollisionLayer::Character, CollisionLayer::Character, true);
        collisionFilter.SetShouldCollide(CollisionLayer::Character, CollisionLayer::Default, true);
        collisionFilter.SetShouldCollide(CollisionLayer::Character, CollisionLayer::CharacterOnlyTrigger, true);
        collisionFilter.SetShouldCollide(CollisionLayer::Character, CollisionLayer::Bullets, true);
        collisionFilter.SetShouldCollide(CollisionLayer::Character, CollisionLayer::Teleporter, true);
        collisionFilter.SetShouldCollide(CollisionLayer::Default,   CollisionLayer::Default, true);
        collisionFilter.SetShouldCollide(CollisionLayer::Default,   CollisionLayer::Teleporter, true);
        collisionFilter.SetShouldCollide(CollisionLayer::Bullets,   CollisionLayer::Default, true);
        
        return collisionFilter;
    }
};
