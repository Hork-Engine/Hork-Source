#include "ComponentManager.h"
#include "World.h"
#include "Component.h"

HK_NAMESPACE_BEGIN

void ComponentManagerBase::RegisterTickFunction(TickFunction const& f)
{
    m_World->RegisterTickFunction(f);
}

void ComponentManagerBase::RegisterDebugDrawFunction(Delegate<void(DebugRenderer& renderer)> const& function)
{
    m_World->RegisterDebugDrawFunction(function);
}

void ComponentManagerBase::InitializeComponent(Component* component)
{
    HK_ASSERT(!component->m_Flags.IsInitialized);

    InvokeBeginPlay(component);
    SubscribeEvents(component);

    component->m_Flags.IsInitialized = true;
}

void ComponentManagerBase::DeinitializeComponent(Component* component)
{
    if (!component->IsInitialized())
        return;

    InvokeEndPlay(component);
    UnsubscribeEvents(component);

    component->m_Flags.IsInitialized = false;
}

Component* ComponentManagerBase::CreateComponentInternal(GameObject* gameObject, ComponentMode componentMode)
{
    HK_IF_NOT_ASSERT(gameObject && !gameObject->m_Flags.IsDestroyed) { return nullptr; }
    HK_IF_NOT_ASSERT(gameObject->GetWorld() == m_World) { return nullptr; }

    if (gameObject->IsStatic() && componentMode != ComponentMode::Static)
        gameObject->SetDynamic(true);

    Component* component;

    auto handle = ConstructComponent(component);

    component->m_Handle = handle;
    component->m_Manager = this;

    if (componentMode == ComponentMode::Dynamic)
        component->m_Flags.IsDynamic = true;

    gameObject->AddComponent(component);

    m_World->m_ComponentsToInitialize.EmplaceBack(handle, m_ComponentTypeID);

    return component;
}

void ComponentManagerBase::DestroyComponent(Component* component)
{
    HK_IF_NOT_ASSERT(component) { return; }
    HK_IF_NOT_ASSERT(component->GetManager() == this) { return; }

    DeinitializeComponent(component);

    if (GameObject* owner = component->GetOwner())
    {
        owner->RemoveComponent(component);
    }

    // TODO: Когда будет гарантировано ранее, что компонент уже в списке, будем использовать метод Add,
    // а пока вызываем SortedInsert, чтобы в списке компоненты не повторялись.
    m_World->m_ComponentsToDelete.SortedInsert(component);
}


HK_NAMESPACE_END
