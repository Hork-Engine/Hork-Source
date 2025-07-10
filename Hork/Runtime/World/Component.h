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

#include <Hork/Core/StringID.h>
#include <Hork/Core/Containers/ObjectStorage.h>

#include "ComponentRTTR.h"

HK_NAMESPACE_BEGIN

class World;
class GameObject;
class ComponentManagerBase;

using ComponentHandle = Handle32<class Component>;

struct ComponentExtendedHandle
{
    ComponentHandle         Handle;
    ComponentTypeID         TypeID{};

                            ComponentExtendedHandle() = default;
                            ComponentExtendedHandle(ComponentHandle handle, ComponentTypeID typeID) : Handle(handle), TypeID(typeID) {}

                            operator ComponentHandle() const    { return Handle; }

                            operator bool() const               { return Handle.operator bool(); }


    HK_NODISCARD uint32_t   Hash() const
    {
        uint32_t h{};
        h = HashTraits::HashCombine(h, Handle);
        h = HashTraits::HashCombine(h, TypeID);
        return h;
    }
};

enum class ComponentMode
{
    Static,
    Dynamic
};

class Component
{
    friend class ComponentManagerBase;
    friend class GameObject;
    
public:
    ComponentHandle         GetHandle() const;

    GameObject*             GetOwner();
    GameObject const*       GetOwner() const;

    World*                  GetWorld();
    World const*            GetWorld() const;

    ComponentManagerBase*   GetManager();

    bool                    IsDynamic() const;

    bool                    IsInitialized() const;

    template <typename ComponentType>
    static ComponentType*   sUpcast(Component* component);

protected:
                            Component() = default;
                            Component(Component const& rhs) = delete;
                            Component(Component&& rhs) = default;

    // There is no need for a virtual destructor since the component is deleted by the component manager.

    Component&              operator=(Component const& rhs) = delete;
    Component&              operator=(Component&& rhs) = delete;

private:
    ComponentHandle         m_Handle;

    union
    {
        uint32_t            m_FlagBits = 0;

        struct
        {
            bool IsInitialized : 1;
            bool IsDynamic : 1;
        } m_Flags;
    };

    GameObject*             m_Owner{};
    ComponentManagerBase*   m_Manager{};
};

namespace ComponentMeta
{
    template <typename ComponentType>
    constexpr ObjectStorageType ComponentStorageType()
    {
        return ObjectStorageType::Compact;
    }
}

HK_NAMESPACE_END

#include "Component.inl"
