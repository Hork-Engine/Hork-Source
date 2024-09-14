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

#include <Hork/Runtime/World/Component.h>
#include <Hork/Runtime/World/World.h>

HK_NAMESPACE_BEGIN

template <typename T>
struct ComponentBinding;

template <typename RetType, typename... Args>
struct ComponentBinding<RetType(Args...)>
{
                        ComponentBinding() = default;

                        template <typename T>
                        ComponentBinding(T* component, RetType (T::*method)(Args...));

                        ComponentBinding(ComponentBinding<RetType(Args...)> const& rhs) = default;
                        ComponentBinding(ComponentBinding<RetType(Args...)>&& rhs) = default;
                        ComponentBinding<RetType(Args...)>& operator=(ComponentBinding<RetType(Args...)> const& rhs) = default;
                        ComponentBinding<RetType(Args...)>& operator=(ComponentBinding<RetType(Args...)>&& rhs) = default;

    void                Clear();

    template <typename T>
    void                Bind(T* component, RetType (T::*method)(Args...));

    RetType             Invoke(World* world, Args... args) const;

protected:
    ComponentHandle     m_Handle;
    ComponentTypeID     m_TypeID;
    RetType(Component::*m_Method)(Args...);
};

template <typename RetType, typename... Args>
template <typename T>
HK_FORCEINLINE ComponentBinding<RetType(Args...)>::ComponentBinding(T* component, RetType (T::*method)(Args...)) :
    m_Handle(component->GetHandle()), m_TypeID(ComponentRTTR::TypeID<T>), m_Method((void(Component::*)(Args...))method)
{}

template <typename RetType, typename... Args>
HK_FORCEINLINE void ComponentBinding<RetType(Args...)>::Clear()
{
    m_Handle = {};
}

template <typename RetType, typename... Args>
template <typename T>
HK_FORCEINLINE void ComponentBinding<RetType(Args...)>::Bind(T* component, RetType (T::*method)(Args...))
{
    m_Handle = component->GetHandle();
    m_TypeID = ComponentRTTR::TypeID<T>;
    m_Method = (void(Component::*)(Args...))method;
}

template <typename RetType, typename... Args>
HK_INLINE RetType ComponentBinding<RetType(Args...)>::Invoke(World* world, Args... args) const
{
    if (m_Handle)
    {
        auto* componentManager = world->TryGetComponentManager(m_TypeID);
        if (componentManager)
        {
            if (auto* component = componentManager->GetComponent(m_Handle))
                return (component->*m_Method)(std::forward<Args>(args)...);
        }
    }
    return RetType();
}

HK_NAMESPACE_END
