#pragma once

#include <Engine/Core/BaseTypes.h>

HK_NAMESPACE_BEGIN

using ComponentTypeID = uint32_t;

template <typename T>
struct ComponentID
{
public:
    // NOTE: ID is used for runtime. For static time use ComponentTypeRegistry::StaticTimeGenerateTypeID<T>
    static const ComponentTypeID ID;
};

struct ComponentTypeRegistry
{
    //static ComponentTypeInfo* Registry;
    //static size_t RegistrySize;

    /// Returns component ID by type
    template <typename T>
    static ComponentTypeID GetComponentTypeID()
    {
        return ComponentID<T>::ID;
    }

    /// Total component types count
    static size_t GetComponentTypesCount()
    {
        return m_IdGen;
    }

    /// Static time ID generation. Do not use at runtime.
    template <typename T>
    static ComponentTypeID StaticTimeGenerateTypeID()
    {
        using ComponentType = typename std::remove_const_t<T>;
        return _GenerateTypeID<ComponentType>();
    }

private:
    template <typename T>
    static ComponentTypeID _GenerateTypeID();

private:
    static ComponentTypeID m_IdGen;
};

template <typename T>
const ComponentTypeID ComponentID<T>::ID = ComponentTypeRegistry::StaticTimeGenerateTypeID<T>();

template <typename T>
ComponentTypeID ComponentTypeRegistry::_GenerateTypeID()
{
    static_assert(!std::is_const<T>::value, "The type must not be constant");

    struct IdGen
    {
        IdGen() :
            ID(m_IdGen++)
        {
#if 0
            if (Id >= RegistrySize)
            {
                auto oldSize = RegistrySize;
                RegistrySize = std::max<size_t>(256, oldSize * 2);
                Registry = (ComponentTypeInfo*)realloc(Registry, RegistrySize * sizeof(ComponentTypeInfo));
                Core::ZeroMem((uint8_t*)Registry + oldSize * sizeof(ComponentTypeInfo), (RegistrySize - oldSize) * sizeof(ComponentTypeInfo));
            }
            Registry[Id].Size = sizeof(T);
            Registry[Id].OnComponentAdded = [](World* world, EntityHandle entityHandle, uint8_t* data)
            {
                world->SendEvent(Hk::ECS::Event::OnComponentAdded<T>(entityHandle, *reinterpret_cast<T*>(data)));
            };
            Registry[Id].OnComponentRemoved = [](World* world, EntityHandle entityHandle, uint8_t* data)
            {
                world->SendEvent(Hk::ECS::Event::OnComponentRemoved<T>(entityHandle, *reinterpret_cast<T*>(data)));
            };
            Registry[Id].Destruct = [](uint8_t* data)
            {
                T* component = std::launder(reinterpret_cast<T*>(data));
                component->~T();
            };
            Registry[Id].Move = [](uint8_t* src, uint8_t* dst)
            {
                new (&dst[0]) T(std::move(*reinterpret_cast<T*>(src)));

                T* component = std::launder(reinterpret_cast<T*>(src));
                component->~T();
            };
#endif
        }

        const ComponentTypeID ID;
    };

    static IdGen generator;
    return generator.ID;
}

HK_NAMESPACE_END
