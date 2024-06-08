#pragma once

#include <Engine/Core/Containers/PageStorage.h>
#include <Engine/Math/Transform.h>

#include "Component.h"

HK_NAMESPACE_BEGIN

class World;
using GameObjectHandle = Handle32<class GameObject>;

struct GameObjectDesc
{
    StringID                Name;
    GameObjectHandle        Parent;
    Float3                  Position;
    Quat                    Rotation;
    Float3                  Scale = Float3(1);
    bool                    AbsolutePosition{};
    bool                    AbsoluteRotation{};
    bool                    AbsoluteScale{};
    bool                    IsDynamic{};
};

class GameObject final
{
    friend class World;
    friend class ComponentManagerBase;
    friend class PageStorage<GameObject>;

public:
    enum
    {
        INPLACE_COMPONENT_COUNT = 8
    };
    using ComponentVector = TSmallVector<Component*, INPLACE_COMPONENT_COUNT>;

    GameObjectHandle        GetHandle() const;
    StringID                GetName() const;

    World*                  GetWorld();
    World const*            GetWorld() const;

    /// Если объект динамический, то к нему можно аттачить и статические, и динамические компоненты.
    /// Если объект статический, то к нему можно аттачить только статические компоненты.
    /// При попытке к статичесеому объекту присоединить динамический компонент, объект автоматически становится динамическим.
    /// Динамический компонент меняет трансформу у своего владельца (управляет им), например DynamicBodyComponent, CharacterControllerComponent и т.п.
    /// Статический компонент не меняет трансформу у соего владельца, но ничто не мешает ему менять трансформы других объектов,
    /// правда для этого нужно учитывать являются ли эти объекты динамическими.
    void                    SetDynamic(bool dynamic);

    bool                    IsStatic() const;
    bool                    IsDynamic() const;

    template <typename ComponentType>
    Handle32<ComponentType> CreateComponent();

    template <typename ComponentType>
    Handle32<ComponentType> CreateComponent(ComponentType*& component);

    template <typename ComponentType>
    ComponentType*          GetComponent();

    template <typename ComponentType>
    Handle32<ComponentType> GetComponentHandle();

    Component*              GetComponent(ComponentTypeID id);

    template <typename ComponentType>
    void                    GetAllComponents(TVector<ComponentType*>& components);

    void                    GetAllComponents(ComponentTypeID id, TVector<Component*>& components);

    ComponentVector const&  GetComponents() const;

    enum class TransformRule
    {
        KeepRelative,
        KeepWorld
    };

    void                    SetParent(GameObjectHandle handle, TransformRule transformRule = TransformRule::KeepRelative);
    void                    SetParent(GameObject* parent, TransformRule transformRule = TransformRule::KeepRelative);
    GameObject*             GetParent();

    struct ChildIterator
    {
        friend class GameObject;

    private:
        GameObject*         m_Object;
        World const*        m_World;
        
    public:
                            ChildIterator(GameObject* first, World const* world);

        GameObject&         operator*();

        GameObject*         operator->();

                            operator GameObject*();

        bool                IsValid() const;

        void                operator++();
    };

    struct ConstChildIterator
    {
        friend class GameObject;

    private:
        GameObject const*   m_Object;
        World const*        m_World;

    public:
                            ConstChildIterator(GameObject const* first, World const* world);

        GameObject const&   operator*() const;

        GameObject const*   operator->() const;

                            operator GameObject const*() const;

        bool                IsValid() const;

        void                operator++();
    };

    ChildIterator           GetChildren();
    ConstChildIterator      GetChildren() const;

    void                    SetPosition(Float3 const& position);
    void                    SetRotation(Quat const& rotation);
    void                    SetScale(Float3 const& scale);
    void                    SetPositionAndRotation(Float3 const& position, Quat const& rotation);
    void                    SetTransform(Float3 const& position, Quat const& rotation, Float3 const& scale);
    void                    SetTransform(Transform const& transform);
    void                    SetAngles(Angl const& angles);
    void                    SetDirection(Float3 const& direction);

    void                    SetWorldPosition(Float3 const& position);
    void                    SetWorldRotation(Quat const& rotation);
    void                    SetWorldScale(Float3 const& scale);
    void                    SetWorldPositionAndRotation(Float3 const& position, Quat const& rotation);
    void                    SetWorldTransform(Float3 const& position, Quat const& rotation, Float3 const& scale);
    void                    SetWorldTransform(Transform const& transform);
    void                    SetWorldAngles(Angl const& angles);
    void                    SetWorldDirection(Float3 const& direction);

    void                    SetAbsolutePosition(bool absolutePosition);
    void                    SetAbsoluteRotation(bool absoluteRotation);
    void                    SetAbsoluteScale(bool absoluteScale);

    bool                    HasAbsolutePosition() const;
    bool                    HasAbsoluteRotation() const;
    bool                    HasAbsoluteScale() const;

    Float3 const&           GetPosition() const;
    Quat const&             GetRotation() const;
    Float3 const&           GetScale() const;
    Float3                  GetRightVector() const;
    Float3                  GetLeftVector() const;
    Float3                  GetUpVector() const;
    Float3                  GetDownVector() const;
    Float3                  GetBackVector() const;
    Float3                  GetForwardVector() const;
    Float3                  GetDirection() const;
    void                    GetVectors(Float3* right, Float3* up, Float3* back) const;

    Float3 const&           GetWorldPosition() const;
    Quat const&             GetWorldRotation() const;
    Float3 const&           GetWorldScale() const;
    Float3x4 const&         GetWorldTransformMatrix() const;
    Float3                  GetWorldRightVector() const;
    Float3                  GetWorldLeftVector() const;
    Float3                  GetWorldUpVector() const;
    Float3                  GetWorldDownVector() const;
    Float3                  GetWorldBackVector() const;
    Float3                  GetWorldForwardVector() const;
    Float3                  GetWorldDirection() const;
    void                    GetWorldVectors(Float3* right, Float3* up, Float3* back) const;

    void                    Rotate(float degrees, Float3 const& normalizedAxis);
    void                    Move(Float3 const& dir);

    void                    UpdateWorldTransform();

    void                    SetLockWorldPositionAndRotation(bool lock);
    
private:
                            GameObject() = default;

                            GameObject(GameObject const& rhs) = delete;
                            GameObject(GameObject&& rhs);

    GameObject&             operator=(GameObject const& rhs) = delete;
    GameObject&             operator=(GameObject&& rhs) = delete;

    void                    AddComponent(Component* component);
    void                    RemoveComponent(Component* component);
    void                    PatchComponentPointer(Component* oldPointer, Component* newPointer);

    void                    LinkToParent();
    void                    UnlinkFromParent();

    GameObjectHandle        m_Handle;

    union
    {
        uint32_t            m_FlagBits = 0;
        struct
        {
            bool            IsDynamic : 1;
            bool            IsDestroyed : 1;
        } m_Flags;
    };

    World*                  m_World = nullptr;

    GameObjectHandle        m_Parent = 0;
    GameObjectHandle        m_FirstChild = 0;
    GameObjectHandle        m_LastChild = 0;
    GameObjectHandle        m_NextSibling = 0;
    GameObjectHandle        m_PrevSibling = 0;
    uint16_t                m_ChildCount = 0;
    uint16_t                m_HierarchyLevel = 0;

    struct TransformData
    {
        GameObject*         Owner{};
        TransformData*      Parent{};
        bool                LockWorldPositionAndRotation{};
        bool                AbsolutePosition{};
        bool                AbsoluteRotation{};
        bool                AbsoluteScale{};

        Float3              Position;
        Quat                Rotation;
        Float3              Scale = Float3(1.0f);

        Float3              WorldPosition;
        Quat                WorldRotation;
        Float3              WorldScale = Float3(1.0f);
        Float3x4            WorldTransform;

        void                UpdateWorldTransformMatrix();

        void                UpdateWorldTransform_r();

        void                UpdateWorldTransform();
    };

    TransformData*          m_TransformData{};

    ComponentVector         m_Components;

    StringID                m_Name;
};

HK_NAMESPACE_END

#include "GameObject.inl"
