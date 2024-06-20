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

#include <Engine/World/WorldInterface.h>
#include <Engine/GameApplication/InputSystem.h>
#include "InputBindings.h"

HK_NAMESPACE_BEGIN

class InputInterface : public WorldInterfaceBase
{
public:
                                InputInterface() {}

    void                        SetActive(bool isActive) { m_IsActive = isActive; }
    bool                        IsActive() const { return m_IsActive; }

    template <typename ComponentType>
    void                        BindInput(Handle32<ComponentType> component, PlayerController player);

    void                        UnbindAll(PlayerController player);

protected:
    virtual void                Initialize() override;
    virtual void                Deinitialize() override;

private:
    void                        Update();

    void                        InvokeAction(StringID name, InputEvent event, PlayerController player);
    void                        InvokeAxis(StringID name, float amount, PlayerController player);

    InputBindings               m_Bindings[ToUnderlying(PlayerController::MAX_PLAYER_CONTROLLERS)];
    bool                        m_IsActive = false;
};

HK_FIND_METHOD(BindInput)

template <typename ComponentType>
HK_INLINE void InputInterface::BindInput(Handle32<ComponentType> component, PlayerController player)
{
    UnbindAll(player);

    if constexpr (!HK_HAS_METHOD(ComponentType, BindInput))
    {
        LOG("Component has no 'BindInput' method\n");
    }
    else
    {
        if (auto* componentPtr = GetWorld()->GetComponent(component))
            componentPtr->BindInput(m_Bindings[ToUnderlying(player)]);
    }
}

HK_NAMESPACE_END
