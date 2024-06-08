#pragma once

#include <Engine/Core/Handle.h>
#include <Engine/Core/StringID.h>

#include "ComponentTypeRegistry.h"

HK_NAMESPACE_BEGIN

class World;

using ComponentHandle = Handle32<class Component>;

struct ComponentExtendedHandle
{
    ComponentHandle         Handle;
    ComponentTypeID         TypeID{};

                            ComponentExtendedHandle() = default;
                            ComponentExtendedHandle(ComponentHandle handle, ComponentTypeID typeID) : Handle(handle), TypeID(typeID) {}

                            operator ComponentHandle() const    { return Handle; }

                            operator bool() const               { return Handle.operator bool(); }
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
    static ComponentType*   Upcast(Component* component);

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

HK_NAMESPACE_END

#include "Component.inl"
