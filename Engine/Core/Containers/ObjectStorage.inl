HK_NAMESPACE_BEGIN

namespace NewEngine
{

template <typename T>
HK_FORCEINLINE ObjectStorage<T>::ObjectStorage(ObjectStorage&& rhs) noexcept :
    m_PageStorage(std::move(rhs.m_PageStorage)),
    m_RandomAccess(std::move(rhs.m_RandomAccess)),
    m_FreeListHead(rhs.m_FreeListHead)
{
    rhs.m_FreeListHead = 0;
}

template <typename T>
HK_FORCEINLINE ObjectStorage<T>& ObjectStorage<T>::operator=(ObjectStorage&& rhs) noexcept
{
    m_PageStorage = std::move(rhs.m_PageStorage);
    m_RandomAccess = std::move(rhs.m_RandomAccess);
    m_FreeListHead = rhs.m_FreeListHead;
    rhs.m_FreeListHead = 0;
    return *this;
}

template <typename T>
HK_INLINE ObjectStorage<T>::~ObjectStorage() = default;

template <typename T>
HK_INLINE Handle32<T> ObjectStorage<T>::CreateObject(T*& newObject)
{
    static_assert(sizeof(T) >= sizeof(m_FreeListHead), "The size of the object must be greater than or equal to sizeof(uint32_t)");

    if (m_FreeListHead)
    {
        auto freeHandle = Handle32<T>(m_FreeListHead);
        auto freeHandleID = freeHandle.GetID();

        m_FreeListHead = *reinterpret_cast<uint32_t*>(m_PageStorage.GetAddress(m_PageStorage.Size()));

        auto version = freeHandle.GetVersion() + 1;
        if (version >= Handle32<T>::MAX_VERSION)
            version = 1;

        auto handle = Handle32<T>(freeHandleID, version);

        newObject = m_PageStorage.EmplaceBack();
        m_RandomAccess[freeHandleID] = newObject;

        return handle;
    }

    uint32_t handleID = m_PageStorage.Size();
    if (handleID >= Handle32<T>::MAX_ID)
        CoreApplication::TerminateWithError("ObjectStorage::Create: Too many objects allocated\n");

    auto handle = Handle32<T>(handleID, 1);

    newObject = m_PageStorage.EmplaceBack();
    m_RandomAccess.Add(newObject);

    return handle;
}

template <typename T>
template <typename HandleFetcher>
HK_INLINE void ObjectStorage<T>::DestroyObject(Handle32<T> handle)
{
    T* movedObject;
    DestroyObject<HandleFetcher>(handle, movedObject);
}

template <typename T>
template <typename HandleFetcher>
HK_INLINE void ObjectStorage<T>::DestroyObject(Handle32<T> handle, T*& movedObject)
{
    uint32_t handleID = handle.GetID();
    HK_ASSERT(m_RandomAccess[handleID]);

    movedObject = &m_PageStorage[m_PageStorage.Size() - 1];

    uint32_t movedObjectID = HandleFetcher::FetchHandle(movedObject).GetID();

    m_PageStorage.RemoveUnsorted(m_RandomAccess[handleID]);

    *reinterpret_cast<uint32_t*>(m_PageStorage.GetAddress(m_PageStorage.Size())) = m_FreeListHead;
    m_FreeListHead = handle.ToUInt32();

    if (movedObjectID != handleID)
    {
        m_RandomAccess[movedObjectID] = m_RandomAccess[handleID];
    }
    else
        movedObject = nullptr;

    m_RandomAccess[handleID] = nullptr;
}

template <typename T>
HK_FORCEINLINE T* ObjectStorage<T>::GetObject(Handle32<T> handle)
{
    return m_RandomAccess[handle.GetID()];
}

template <typename T>
HK_FORCEINLINE T const* ObjectStorage<T>::GetObject(Handle32<T> handle) const
{
    return m_RandomAccess[handle.GetID()];
}

template <typename T>
HK_FORCEINLINE uint32_t ObjectStorage<T>::Size() const
{
    return m_PageStorage.Size();
}

template <typename T>
HK_FORCEINLINE T* ObjectStorage<T>::At(uint32_t index)
{
    return &m_PageStorage[index];
}

template <typename T>
HK_FORCEINLINE T const* ObjectStorage<T>::At(uint32_t index) const
{
    return &m_PageStorage[index];
}

template <typename T>
template <typename Visitor>
HK_FORCEINLINE void ObjectStorage<T>::Iterate(Visitor& visitor)
{
    m_PageStorage.Iterate(visitor);
}

template <typename T>
template <typename Visitor>
HK_FORCEINLINE void ObjectStorage<T>::IterateBatches(Visitor& visitor)
{
    m_PageStorage.IterateBatches(visitor);
}

template <typename T>
HK_FORCEINLINE typename ObjectStorage<T>::Iterator ObjectStorage<T>::GetObjects()
{
    return Iterator(0, m_PageStorage.Size(), m_PageStorage);
}

template <typename T>
HK_FORCEINLINE typename ObjectStorage<T>::ConstIterator ObjectStorage<T>::GetObjects() const
{
    return ConstIterator(0, m_PageStorage.Size(), m_PageStorage);
}

template <typename T>
HK_FORCEINLINE ObjectStorage<T>::Iterator::Iterator(uint32_t startIndex, uint32_t endIndex, PageStorage<T>& pageStorage) :
    m_Index(startIndex), m_EndIndex(endIndex), m_PageStorage(pageStorage)
{}

template <typename T>
HK_FORCEINLINE T& ObjectStorage<T>::Iterator::operator*()
{
    return m_PageStorage[m_Index];
}

template <typename T>
HK_FORCEINLINE T* ObjectStorage<T>::Iterator::operator->()
{
    return &m_PageStorage[m_Index];
}

template <typename T>
HK_FORCEINLINE ObjectStorage<T>::Iterator::operator T*()
{
    return &m_PageStorage[m_Index];
}

template <typename T>
HK_FORCEINLINE bool ObjectStorage<T>::Iterator::IsValid() const
{
    return m_Index < m_EndIndex;
}

template <typename T>
HK_FORCEINLINE uint32_t ObjectStorage<T>::Iterator::GetIndex() const
{
    return m_Index;
}

template <typename T>
HK_FORCEINLINE void ObjectStorage<T>::Iterator::operator++()
{
    m_Index++;
}

template <typename T>
HK_FORCEINLINE ObjectStorage<T>::ConstIterator::ConstIterator(uint32_t startIndex, uint32_t endIndex, PageStorage<T> const& pageStorage) :
    m_Index(startIndex), m_EndIndex(endIndex), m_PageStorage(pageStorage)
{}

template <typename T>
HK_FORCEINLINE T const& ObjectStorage<T>::ConstIterator::operator*() const
{
    return m_PageStorage[m_Index];
}

template <typename T>
HK_FORCEINLINE T const* ObjectStorage<T>::ConstIterator::operator->() const
{
    return &m_PageStorage[m_Index];
}

template <typename T>
HK_FORCEINLINE ObjectStorage<T>::ConstIterator::operator T const*() const
{
    return &m_PageStorage[m_Index];
}

template <typename T>
HK_FORCEINLINE bool ObjectStorage<T>::ConstIterator::IsValid() const
{
    return m_Index < m_EndIndex;
}

template <typename T>
HK_FORCEINLINE uint32_t ObjectStorage<T>::ConstIterator::GetIndex() const
{
    return m_Index;
}

template <typename T>
HK_FORCEINLINE void ObjectStorage<T>::ConstIterator::operator++()
{
    m_Index++;
}

} // namespace NewEngine

HK_NAMESPACE_END
