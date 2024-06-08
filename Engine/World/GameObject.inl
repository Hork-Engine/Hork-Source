HK_NAMESPACE_BEGIN

HK_FORCEINLINE GameObject::GameObject(GameObject&& rhs) :
    m_Handle(rhs.m_Handle),
    m_World(rhs.m_World),
    m_FlagBits(rhs.m_FlagBits),
    m_Parent(rhs.m_Parent),
    m_FirstChild(rhs.m_FirstChild),
    m_LastChild(rhs.m_LastChild),
    m_NextSibling(rhs.m_NextSibling),
    m_PrevSibling(rhs.m_PrevSibling),
    m_ChildCount(rhs.m_ChildCount),
    m_HierarchyLevel(rhs.m_HierarchyLevel),
    m_TransformData(rhs.m_TransformData),
    m_Components(std::move(rhs.m_Components)),
    m_Name(rhs.m_Name)
{
    rhs.m_Components.Clear();
    rhs.m_Name = {};

    // Patch pointer
    for (auto component : m_Components)
        component->m_Owner = this;
}

HK_FORCEINLINE GameObjectHandle GameObject::GetHandle() const
{
    return m_Handle;
}

HK_FORCEINLINE StringID GameObject::GetName() const
{
    return m_Name;
}

HK_FORCEINLINE World* GameObject::GetWorld()
{
    return m_World;
}

HK_FORCEINLINE World const* GameObject::GetWorld() const
{
    return m_World;
}

HK_FORCEINLINE bool GameObject::IsStatic() const
{
    return !m_Flags.IsDynamic;
}

HK_FORCEINLINE bool GameObject::IsDynamic() const
{
    return m_Flags.IsDynamic;
}

template <typename ComponentType>
HK_FORCEINLINE Handle32<ComponentType> GameObject::CreateComponent()
{
    return m_World->GetComponentManager<ComponentType>().CreateComponent(this);
}

template <typename ComponentType>
HK_FORCEINLINE Handle32<ComponentType> GameObject::CreateComponent(ComponentType*& component)
{
    return m_World->GetComponentManager<ComponentType>().CreateComponent(this, component);
}

template <typename ComponentType>
HK_FORCEINLINE ComponentType* GameObject::GetComponent()
{
    return static_cast<ComponentType*>(GetComponent(ComponentTypeRegistry::GetComponentTypeID<ComponentType>()));
}

template <typename ComponentType>
HK_FORCEINLINE Handle32<ComponentType> GameObject::GetComponentHandle()
{
    auto component = GetComponent(ComponentTypeRegistry::GetComponentTypeID<ComponentType>());
    if (!component)
        return {};
    return static_cast<Handle32<ComponentType>>(component->GetHandle());
}

template <typename ComponentType>
HK_FORCEINLINE void GameObject::GetAllComponents(TVector<ComponentType*>& components)
{
    GetAllComponents(ComponentTypeRegistry::GetComponentTypeID<ComponentType>(),
        static_cast<TVector<ComponenBase*>&>(components));
}

HK_FORCEINLINE GameObject::ComponentVector const& GameObject::GetComponents() const
{
    return m_Components;
}

HK_FORCEINLINE GameObject::ChildIterator::ChildIterator(GameObject* first, World const* world) :
    m_Object(first), m_World(world)
{}

HK_FORCEINLINE GameObject& GameObject::ChildIterator::operator*()
{
    return *m_Object;
}

HK_FORCEINLINE GameObject* GameObject::ChildIterator::operator->()
{
    return m_Object;
}

HK_FORCEINLINE GameObject::ChildIterator::operator GameObject*()
{
    return m_Object;
}

HK_FORCEINLINE bool GameObject::ChildIterator::IsValid() const
{
    return m_Object != nullptr;
}

HK_FORCEINLINE GameObject::ConstChildIterator::ConstChildIterator(GameObject const* first, World const* world) :
    m_Object(first), m_World(world)
{}

HK_FORCEINLINE GameObject const& GameObject::ConstChildIterator::operator*() const
{
    return *m_Object;
}

HK_FORCEINLINE GameObject const* GameObject::ConstChildIterator::operator->() const
{
    return m_Object;
}

HK_FORCEINLINE GameObject::ConstChildIterator::operator GameObject const*() const
{
    return m_Object;
}

HK_FORCEINLINE bool GameObject::ConstChildIterator::IsValid() const
{
    return m_Object != nullptr;
}

HK_FORCEINLINE void GameObject::SetPosition(Float3 const& position)
{
    m_TransformData->Position = position;
}

HK_FORCEINLINE void GameObject::SetRotation(Quat const& rotation)
{
    m_TransformData->Rotation = rotation;
}

HK_FORCEINLINE void GameObject::SetScale(Float3 const& scale)
{
    m_TransformData->Scale = scale;
}

HK_FORCEINLINE void GameObject::SetPositionAndRotation(Float3 const& position, Quat const& rotation)
{
    m_TransformData->Position = position;
    m_TransformData->Rotation = rotation;
}

HK_FORCEINLINE void GameObject::SetTransform(Float3 const& position, Quat const& rotation, Float3 const& scale)
{
    m_TransformData->Position = position;
    m_TransformData->Rotation = rotation;
    m_TransformData->Scale = scale;
}

HK_FORCEINLINE void GameObject::SetTransform(Transform const& transform)
{
    m_TransformData->Position = transform.Position;
    m_TransformData->Rotation = transform.Rotation;
    m_TransformData->Scale = transform.Scale;
}

HK_FORCEINLINE void GameObject::SetAngles(Angl const& angles)
{
    m_TransformData->Rotation = angles.ToQuat();
}

HK_FORCEINLINE void GameObject::SetDirection(Float3 const& direction)
{
    SetRotation(Math::MakeRotationFromDir(direction));
}

HK_FORCEINLINE void GameObject::SetWorldPosition(Float3 const& position)
{
    m_TransformData->WorldPosition = position;
    m_TransformData->UpdateWorldTransformMatrix();

    if (m_TransformData->Parent && !m_TransformData->AbsolutePosition)
    {
        m_TransformData->Position = m_TransformData->Parent->WorldTransform.Inversed() * position;
    }
    else
    {
        m_TransformData->Position = position;
    }
}

HK_FORCEINLINE void GameObject::SetWorldRotation(Quat const& rotation)
{
    m_TransformData->WorldRotation = rotation;
    m_TransformData->UpdateWorldTransformMatrix();

    if (m_TransformData->Parent && !m_TransformData->AbsoluteRotation)
    {
        m_TransformData->Rotation = m_TransformData->Parent->WorldRotation.Inversed() * rotation;
    }
    else
    {
        m_TransformData->Rotation = rotation;
    }
}

HK_FORCEINLINE void GameObject::SetWorldScale(Float3 const& scale)
{
    m_TransformData->WorldScale = scale;
    m_TransformData->UpdateWorldTransformMatrix();

    if (m_TransformData->Parent && !m_TransformData->AbsoluteScale)
    {
        m_TransformData->Scale = scale / m_TransformData->Parent->WorldScale;
    }
    else
    {
        m_TransformData->Scale = scale;
    }
}

HK_FORCEINLINE void GameObject::SetWorldPositionAndRotation(Float3 const& position, Quat const& rotation)
{
    m_TransformData->WorldPosition = position;
    m_TransformData->WorldRotation = rotation;
    m_TransformData->UpdateWorldTransformMatrix();

    if (m_TransformData->Parent)
    {
        m_TransformData->Position = m_TransformData->AbsolutePosition ? position : m_TransformData->Parent->WorldTransform.Inversed() * position;
        m_TransformData->Rotation = m_TransformData->AbsoluteRotation ? rotation : m_TransformData->Parent->WorldRotation.Inversed() * rotation;
    }
    else
    {
        m_TransformData->Position = position;
        m_TransformData->Rotation = rotation;
    }
}

HK_FORCEINLINE void GameObject::SetWorldTransform(Float3 const& position, Quat const& rotation, Float3 const& scale)
{
    m_TransformData->WorldPosition = position;
    m_TransformData->WorldRotation = rotation;
    m_TransformData->WorldScale    = scale;
    m_TransformData->UpdateWorldTransformMatrix();

    if (m_TransformData->Parent)
    {
        m_TransformData->Position = m_TransformData->AbsolutePosition ? position : m_TransformData->Parent->WorldTransform.Inversed() * position;
        m_TransformData->Rotation = m_TransformData->AbsoluteRotation ? rotation : m_TransformData->Parent->WorldRotation.Inversed() * rotation;
        m_TransformData->Scale    = m_TransformData->AbsoluteScale    ? scale    : scale / m_TransformData->Parent->WorldScale;
    }
    else
    {
        m_TransformData->Position = position;
        m_TransformData->Rotation = rotation;
        m_TransformData->Scale    = scale;
    }
}

HK_FORCEINLINE void GameObject::SetWorldTransform(Transform const& transform)
{
    SetWorldTransform(transform.Position, transform.Rotation, transform.Scale);
}

HK_FORCEINLINE void GameObject::SetWorldAngles(Angl const& angles)
{
    SetWorldRotation(angles.ToQuat());
}

HK_FORCEINLINE void GameObject::SetWorldDirection(Float3 const& direction)
{
    SetWorldRotation(Math::MakeRotationFromDir(direction));
}

HK_FORCEINLINE void GameObject::SetAbsolutePosition(bool absolutePosition)
{
    m_TransformData->AbsolutePosition = absolutePosition;
}

HK_FORCEINLINE void GameObject::SetAbsoluteRotation(bool absoluteRotation)
{
    m_TransformData->AbsoluteRotation = absoluteRotation;
}

HK_FORCEINLINE void GameObject::SetAbsoluteScale(bool absoluteScale)
{
    m_TransformData->AbsoluteScale = absoluteScale;
}

HK_FORCEINLINE bool GameObject::HasAbsolutePosition() const
{
    return m_TransformData->AbsolutePosition;
}

HK_FORCEINLINE bool GameObject::HasAbsoluteRotation() const
{
    return m_TransformData->AbsoluteRotation;
}

HK_FORCEINLINE bool GameObject::HasAbsoluteScale() const
{
    return m_TransformData->AbsoluteScale;
}

HK_FORCEINLINE Float3 const& GameObject::GetPosition() const
{
    return m_TransformData->Position;
}

HK_FORCEINLINE Quat const& GameObject::GetRotation() const
{
    return m_TransformData->Rotation;
}

HK_FORCEINLINE Float3 const& GameObject::GetScale() const
{
    return m_TransformData->Scale;
}

HK_FORCEINLINE Float3 GameObject::GetRightVector() const
{
    return m_TransformData->Rotation.XAxis();
}

HK_FORCEINLINE Float3 GameObject::GetLeftVector() const
{
    return -m_TransformData->Rotation.XAxis();
}

HK_FORCEINLINE Float3 GameObject::GetUpVector() const
{
    return m_TransformData->Rotation.YAxis();
}

HK_FORCEINLINE Float3 GameObject::GetDownVector() const
{
    return -m_TransformData->Rotation.YAxis();
}

HK_FORCEINLINE Float3 GameObject::GetBackVector() const
{
    return m_TransformData->Rotation.ZAxis();
}

HK_FORCEINLINE Float3 GameObject::GetForwardVector() const
{
    return -m_TransformData->Rotation.ZAxis();
}

HK_FORCEINLINE Float3 GameObject::GetDirection() const
{
    return GetForwardVector();
}

HK_FORCEINLINE void GameObject::GetVectors(Float3* right, Float3* up, Float3* back) const
{
    Math::GetTransformVectors(m_TransformData->Rotation, right, up, back);
}

HK_FORCEINLINE Float3 const& GameObject::GetWorldPosition() const
{
    return m_TransformData->WorldPosition;
}

HK_FORCEINLINE Quat const& GameObject::GetWorldRotation() const
{
    return m_TransformData->WorldRotation;
}

HK_FORCEINLINE Float3 const& GameObject::GetWorldScale() const
{
    return m_TransformData->WorldScale;
}

HK_FORCEINLINE Float3x4 const& GameObject::GetWorldTransformMatrix() const
{
    return m_TransformData->WorldTransform;
}

HK_FORCEINLINE Float3 GameObject::GetWorldRightVector() const
{
    return m_TransformData->WorldRotation.XAxis();
}

HK_FORCEINLINE Float3 GameObject::GetWorldLeftVector() const
{
    return -m_TransformData->WorldRotation.XAxis();
}

HK_FORCEINLINE Float3 GameObject::GetWorldUpVector() const
{
    return m_TransformData->WorldRotation.YAxis();
}

HK_FORCEINLINE Float3 GameObject::GetWorldDownVector() const
{
    return -m_TransformData->WorldRotation.YAxis();
}

HK_FORCEINLINE Float3 GameObject::GetWorldBackVector() const
{
    return m_TransformData->WorldRotation.ZAxis();
}

HK_FORCEINLINE Float3 GameObject::GetWorldForwardVector() const
{
    return -m_TransformData->WorldRotation.ZAxis();
}

HK_FORCEINLINE Float3 GameObject::GetWorldDirection() const
{
    return GetWorldForwardVector();
}

HK_FORCEINLINE void GameObject::GetWorldVectors(Float3* right, Float3* up, Float3* back) const
{
    Math::GetTransformVectors(m_TransformData->WorldRotation, right, up, back);
}

HK_FORCEINLINE void GameObject::Rotate(float degrees, Float3 const& normalizedAxis)
{
    float s, c;

    Math::DegSinCos(degrees * 0.5f, s, c);

    m_TransformData->Rotation = Quat(c, s * normalizedAxis.X, s * normalizedAxis.Y, s * normalizedAxis.Z) * m_TransformData->Rotation;
    m_TransformData->Rotation.NormalizeSelf();
}

HK_FORCEINLINE void GameObject::Move(Float3 const& dir)
{
    m_TransformData->Position += dir;
}

HK_FORCEINLINE void GameObject::TransformData::UpdateWorldTransformMatrix()
{
    WorldTransform.Compose(WorldPosition, WorldRotation.ToMatrix3x3(), WorldScale);
}

HK_INLINE void GameObject::TransformData::UpdateWorldTransform_r()
{
    if (Parent)
    {
        Parent->UpdateWorldTransform_r();

        WorldPosition = AbsolutePosition ? Position : Parent->WorldTransform * Position;
        WorldRotation = AbsoluteRotation ? Rotation : Parent->WorldRotation * Rotation;
        WorldScale    = AbsoluteScale    ? Scale    : Parent->WorldScale * Scale;
    }
    else
    {
        WorldPosition = Position;
        WorldRotation = Rotation;
        WorldScale    = Scale;
    }

    UpdateWorldTransformMatrix();
}

HK_FORCEINLINE void GameObject::TransformData::UpdateWorldTransform()
{
    if (Parent)
    {
        WorldPosition = AbsolutePosition ? Position : Parent->WorldTransform * Position;
        WorldRotation = AbsoluteRotation ? Rotation : Parent->WorldRotation * Rotation;
        WorldScale    = AbsoluteScale    ? Scale    : Parent->WorldScale * Scale;
    }
    else
    {
        WorldPosition = Position;
        WorldRotation = Rotation;
        WorldScale    = Scale;
    }

    UpdateWorldTransformMatrix();
}

HK_NAMESPACE_END
