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

#include <Hork/Runtime/World/Modules/Physics/Components/CharacterControllerComponent.h>
#include <Hork/Runtime/World/Modules/Physics/Components/DynamicBodyComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/CameraComponent.h>
#include <Hork/Runtime/World/Modules/Render/Components/MeshComponent.h>
#include <Hork/Runtime/GameApplication/GameApplication.h>

using namespace Hk;

class TeleporterComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    struct TeleportPoint
    {
        Float3 Position;
        Quat Rotation;
    };

    Array<TeleportPoint, 2> TeleportPoints;

    void OnBeginOverlap(BodyComponent* body)
    {
        auto& dest = TeleportPoints[GameApplication::sGetRandom().Get() & 1];

        if (auto character = sUpcast<CharacterControllerComponent>(body))
        {
            character->SetWorldPosition(dest.Position);

            // OnBeginOverlap срабатывает после того, как обновлены глобальные трансформы
            // Чтобы получить актуальную позицию дочерних объектов (в т.ч. привязанной камеры) мы должны пересчитать их глобальные трансформации.
            character->GetOwner()->UpdateChildrenWorldTransform();

            // Также скипаем интерполяцию между трансформами, поскольку телепорт должен происходить мгновенно.
            // То же самое, по идее, нужно делать и с динамическими мешами...
            if (auto cameraOwner = character->GetOwner()->FindChildren(StringID("Camera")))
            {
                cameraOwner->SetWorldRotation(dest.Rotation);
                if (auto camera = cameraOwner->GetComponent<CameraComponent>())
                    camera->SkipInterpolation();
            }
        }

        if (auto rigidbody = sUpcast<DynamicBodyComponent>(body))
        {
            rigidbody->SetWorldPosition(dest.Position);
            rigidbody->SetWorldRotation(dest.Rotation);

            if (auto dynamicMesh = rigidbody->GetOwner()->GetComponent<DynamicMeshComponent>())
            {
                dynamicMesh->SkipInterpolation();
            }
        }
    }
};
